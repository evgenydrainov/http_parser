#include "http_parser.h"
#include <assert.h>

#define STR_FMT "%.*s"
#define STR_ARG(str) str.count, str.data
#define STR(s) ((string) {s, sizeof(s) - 1})

typedef struct {
    const char* data;
    size_t count;
} string;

// Only LF
// TODO: handle CRLF and CR
static string eat_line(string* str) {
    string result = {str->data, 0};
    while (str->count > 0 && *str->data != '\n') {
        str->data++;
        str->count--;
        result.count++;
    }

    // Skip newline character.
    if (str->count > 0) {
        str->data++;
        str->count--;
    }

    return result;
}

static string eat_header_name(string* str) {
    string result = {str->data, 0};
    while (str->count > 0 && *str->data != ':') {
        str->data++;
        str->count--;
        result.count++;
    }

    // Skip colon.
    if (str->count > 0) {
        str->data++;
        str->count--;
    }

    return result;
}

static bool is_whitespace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n'; // Should '\r' be here?
}

static bool is_numeric(char ch) {
    return ch >= '0' && ch <= '9';
}

static bool is_hexadecimal(char ch) {
    return is_numeric(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static string eat_word(string* str) {
    string result = {str->data, 0};
    while (str->count > 0 && !is_whitespace(*str->data)) {
        str->data++;
        str->count--;
        result.count++;
    }
    return result;
}

static void eat_whitespace(string* str) {
    while (str->count > 0 && is_whitespace(*str->data)) {
        str->data++;
        str->count--;
    }
}

static uint16_t string_to_u16(string str, bool* done) {
    uint16_t result = 0;
    for (size_t i = 0; i < str.count; i++) {
        if (!is_numeric(str.data[i])) {
            *done = false;
            return 0;
        }

        result *= 10;
        result += str.data[i] - '0';
    }

    *done = true;
    return result;
}

static uint32_t hex_to_u32(string str, bool* done) {
    uint32_t result = 0;
    for (size_t i = 0; i < str.count; i++) {
        if (!is_hexadecimal(str.data[i])) {
            *done = false;
            return 0;
        }

        result *= 16;
        
        if (is_numeric(str.data[i])) {
            result += str.data[i] - '0';
        } else if (str.data[i] >= 'a' && str.data[i] <= 'f') {
            result += str.data[i] - 'a';
        } else if (str.data[i] >= 'A' && str.data[i] <= 'F') {
            result += str.data[i] - 'A';
        } else {
            assert(false);
        }
    }

    *done = true;
    return result;
}

static bool strings_match(string a, string b) {
    if (a.count != b.count) {
        return false;
    }
    for (size_t i = 0; i < a.count; i++) {
        if (a.data[i] != b.data[i]) {
            return false;
        }
    }
    return true;
}

static bool string_contains_char(string str, char ch) {
    for (size_t i = 0; i < str.count; i++) {
        if (str.data[i] == ch) {
            return true;
        }
    }
    return false;
}

static bool starts_with(string str, string prefix) {
    if (str.count < prefix.count) {
        return false;
    }
    str.count = prefix.count;
    return strings_match(str, prefix);
}

static http_parsing_result_t parse_protocol_version(string* status_line, string* out_protocol_version) {
    eat_whitespace(status_line);
    string protocol_version = eat_word(status_line);

    if (protocol_version.count == 0) {
        return PARSING_RES_NOT_ENOUGH_DATA;
    }

    if (!(strings_match(protocol_version, STR("HTTP/1.1")) || strings_match(protocol_version, STR("HTTP/1.0")))) {
        if (starts_with(STR("HTTP/1."), protocol_version)) {
            if (status_line->count > 0) {
                // If string is not over, then error
                return PARSING_RES_FAILED;
            }
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        // Unknown protocol version
        return PARSING_RES_FAILED;
    }

    *out_protocol_version = protocol_version;
    return PARSING_RES_SUCCEEDED;
}

static http_parsing_result_t parse_headers(string* text,
                                           http_header_t* headers_buf, size_t headers_max_len,
                                           size_t* out_num_headers_written) {
    if (text->count == 0) {
        return PARSING_RES_NOT_ENOUGH_DATA;
    }

    size_t header_index = 0;
    size_t num_headers_written = 0;

    while (text->count > 0) {
        string header_line = eat_line(text);

        if (header_line.count == 0) {
            // Encountered a blank line
            *out_num_headers_written = num_headers_written;
            return PARSING_RES_SUCCEEDED;
        }

        eat_whitespace(&header_line);

        if (!string_contains_char(header_line, ':')) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        string header_name = eat_header_name(&header_line);

        if (header_name.count == 0) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        eat_whitespace(&header_line);
        string header_value = header_line;

        if (header_value.count == 0) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        if (header_index < headers_max_len) {
            headers_buf[header_index].name     = header_name.data;
            headers_buf[header_index].name_len = header_name.count;

            headers_buf[header_index].value     = header_value.data;
            headers_buf[header_index].value_len = header_value.count;

            num_headers_written++;
        } else {
            // Ran out of memory.
            return PARSING_RES_NOT_ENOUGH_MEMORY;
        }

        header_index++;
    }

    // Didn't encounter a blank line
    return PARSING_RES_NOT_ENOUGH_DATA;
}

http_parsing_result_t http_parse_response(const char *text_data, size_t text_len,
                                          http_header_t *headers_buf, size_t headers_max_len,
                                          http_response_t *out_resp) {
    assert(text_data);
    assert(headers_buf);
    assert(out_resp);

    *out_resp = (http_response_t) {0};
    
    string text = {text_data, text_len};

    // Read status line.
    {
        string status_line = eat_line(&text);

        // Protocol version.
        string protocol_version;
        http_parsing_result_t res = parse_protocol_version(&status_line, &protocol_version);
        if (res != PARSING_RES_SUCCEEDED) {
            if (res == PARSING_RES_NOT_ENOUGH_DATA) {
                if (text.count > 0) {
                    // 
                    // If data is incomplete at this point and there's more data then it's an error
                    // (leftover data in same line should've been handled in parse_protocol_version)
                    // 
                    return PARSING_RES_FAILED;
                }
            }
            return res;
        }

        out_resp->protocol     = protocol_version.data;
        out_resp->protocol_len = protocol_version.count;

        // Status code.
        eat_whitespace(&status_line);
        string status_code = eat_word(&status_line);

        if (status_code.count == 0) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        bool done;
        out_resp->status_code = string_to_u16(status_code, &done);
        if (!done) {
            return PARSING_RES_FAILED;
        }

        // Remaining line is the status text.
        eat_whitespace(&status_line);
        string status_text = status_line;

        // Maybe status text can be empty, so don't error

        out_resp->status_text     = status_text.data;
        out_resp->status_text_len = status_text.count;
    }

    // Read headers.
    {
        size_t num_headers_written;
        http_parsing_result_t res = parse_headers(&text, headers_buf, headers_max_len, &num_headers_written);
        if (res != PARSING_RES_SUCCEEDED) {
            if (res == PARSING_RES_NOT_ENOUGH_DATA) {
                if (text.count > 0) {
                    // Same situation like when parsing protocol version
                    return PARSING_RES_FAILED;
                }
            }
            return res;
        }

        out_resp->headers     = headers_buf;
        out_resp->headers_len = num_headers_written;
    }

    // Remaining text is the body.
    {
        string body = text;

        // Body is optional, so don't check for empty

        out_resp->body     = body.data;
        out_resp->body_len = body.count;
    }

    return PARSING_RES_SUCCEEDED;
}

http_parsing_result_t http_parse_request(const char *text_data, size_t text_len,
                                         http_header_t *headers_buf, size_t headers_max_len,
                                         http_request_t *out_req) {
    assert(text_data);
    assert(headers_buf);
    assert(out_req);

    *out_req = (http_request_t) {0};
    
    string text = {text_data, text_len};

    // Read status line.
    {
        string status_line = eat_line(&text);

        // Method.
        eat_whitespace(&status_line);
        string method = eat_word(&status_line);
        
        if (method.count == 0) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        out_req->method     = method.data;
        out_req->method_len = method.count;

        // Target.
        eat_whitespace(&status_line);
        string target = eat_word(&status_line);
        
        if (target.count == 0) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        out_req->target     = target.data;
        out_req->target_len = target.count;

        // Protocol version.
        string protocol_version;
        http_parsing_result_t res = parse_protocol_version(&status_line, &protocol_version);
        if (res != PARSING_RES_SUCCEEDED) {
            if (res == PARSING_RES_NOT_ENOUGH_DATA) {
                if (text.count > 0) {
                    // 
                    // If data is incomplete at this point and there's more data then it's an error
                    // (leftover data in same line should've been handled in parse_protocol_version)
                    // 
                    return PARSING_RES_FAILED;
                }
            }
            return res;
        }

        out_req->protocol     = protocol_version.data;
        out_req->protocol_len = protocol_version.count;
    }

    // Read headers.
    {
        size_t num_headers_written;
        http_parsing_result_t res = parse_headers(&text, headers_buf, headers_max_len, &num_headers_written);
        if (res != PARSING_RES_SUCCEEDED) {
            if (res == PARSING_RES_NOT_ENOUGH_DATA) {
                if (text.count > 0) {
                    // Same situation like when parsing protocol version
                    return PARSING_RES_FAILED;
                }
            }
            return res;
        }

        out_req->headers     = headers_buf;
        out_req->headers_len = num_headers_written;
    }

    // Remaining text is the body.
    {
        string body = text;

        // Body is optional, so don't check for empty

        out_req->body     = body.data;
        out_req->body_len = body.count;
    }

    return PARSING_RES_SUCCEEDED;
}

http_parsing_result_t http_decode_chunked(const char* body_data, size_t body_len,
                                          char* buf, size_t buf_len,
                                          size_t* out_decoded_len) {
    assert(body_data);
    assert(buf);
    assert(out_decoded_len);

    string body = {body_data, body_len};

    size_t decoded_len = 0;

    while (body.count > 0) {
        string length_str = eat_line(&body);

        bool done;
        uint32_t length = hex_to_u32(length_str, &done);
        if (!done) {
            return PARSING_RES_FAILED;
        }

        // Stop when encounter zero
        if (length == 0) {
            // There has to be another empty line
            if (body.count == 0) {
                return PARSING_RES_NOT_ENOUGH_DATA;
            }

            string empty_line = eat_line(&body);
            if (empty_line.count == 0) {
                *out_decoded_len = decoded_len;
                return PARSING_RES_SUCCEEDED;
            } else {
                return PARSING_RES_FAILED;
            }
        }

        if (body.count < length) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }

        for (size_t i = 0; i < length; i++) {
            if (decoded_len >= buf_len) {
                return PARSING_RES_NOT_ENOUGH_MEMORY;
            }
            buf[decoded_len] = body.data[i];
            decoded_len++;
        }

        body.data += length;
        body.count -= length;

        // Skip newline
        // TODO: CRLF
        if (body.count == 0) {
            return PARSING_RES_NOT_ENOUGH_DATA;
        }
        if (*body.data != '\n') {
            return PARSING_RES_FAILED;
        }
        body.data++;
        body.count--;
    }

    // Didn't find a line with a zero
    return PARSING_RES_NOT_ENOUGH_DATA;
}
