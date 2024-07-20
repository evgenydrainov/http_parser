#include "http_parser.h"

#include <stdio.h>
#include <string.h>

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

// 
// TODO: Test requests.
// 

#define STR(s) ((string) {s, sizeof(s) - 1})

typedef struct {
    const char* data;
    size_t count;
} string;

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

#define my_assert(expr) do { while (!(expr)) { printf("ASSERTION FAILED: "__FILE__":%i: %s\n", __LINE__, #expr); exit(1); } } while (0)

static void test_response_full() {
    char text[] = 
        "HTTP/1.1 403 Forbidden\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5, max=1000\n"
        "Connection: Keep-Alive\n"
        "Age: 3464\n"
        "Date: Wed 10, Aug 2016 09:46:25 GMT\n"
        "X-Cache-Info: caching\n"
        "Content-Length: 220\n"
        "\n"
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML\n"
        "2.0//EN\">\n"
        "(more data)";

    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_SUCCEEDED);
    my_assert(strings_match((string){response.protocol, response.protocol_len}, STR("HTTP/1.1")));
    my_assert(response.status_code == 403);
    my_assert(strings_match((string){response.status_text, response.status_text_len}, STR("Forbidden")));
    my_assert(response.headers_len == 9);

    my_assert(strings_match((string){response.headers[0].name, response.headers[0].name_len}, STR("Server")));
    my_assert(strings_match((string){response.headers[1].name, response.headers[1].name_len}, STR("Content-Type")));
    my_assert(strings_match((string){response.headers[2].name, response.headers[2].name_len}, STR("Date")));
    my_assert(strings_match((string){response.headers[3].name, response.headers[3].name_len}, STR("Keep-Alive")));
    my_assert(strings_match((string){response.headers[4].name, response.headers[4].name_len}, STR("Connection")));
    my_assert(strings_match((string){response.headers[5].name, response.headers[5].name_len}, STR("Age")));
    my_assert(strings_match((string){response.headers[6].name, response.headers[6].name_len}, STR("Date")));
    my_assert(strings_match((string){response.headers[7].name, response.headers[7].name_len}, STR("X-Cache-Info")));
    my_assert(strings_match((string){response.headers[8].name, response.headers[8].name_len}, STR("Content-Length")));

    my_assert(strings_match((string){response.headers[0].value, response.headers[0].value_len}, STR("Apache")));
    my_assert(strings_match((string){response.headers[1].value, response.headers[1].value_len}, STR("text/html; charset=iso-8859-1")));
    my_assert(strings_match((string){response.headers[2].value, response.headers[2].value_len}, STR("Wed, 10 Aug 2016 09:23:25 GMT")));
    my_assert(strings_match((string){response.headers[3].value, response.headers[3].value_len}, STR("timeout-=5, max=1000")));
    my_assert(strings_match((string){response.headers[4].value, response.headers[4].value_len}, STR("Keep-Alive")));
    my_assert(strings_match((string){response.headers[5].value, response.headers[5].value_len}, STR("3464")));
    my_assert(strings_match((string){response.headers[6].value, response.headers[6].value_len}, STR("Wed 10, Aug 2016 09:46:25 GMT")));
    my_assert(strings_match((string){response.headers[7].value, response.headers[7].value_len}, STR("caching")));
    my_assert(strings_match((string){response.headers[8].value, response.headers[8].value_len}, STR("220")));
}

static void test_response_invalid_protocol() {
    char text[] = 
        "FOOBAR 403 Forbidden\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5, max=1000\n"
        "Connection: Keep-Alive\n"
        "Age: 3464\n"
        "Date: Wed 10, Aug 2016 09:46:25 GMT\n"
        "X-Cache-Info: caching\n"
        "Content-Length: 220\n"
        "\n"
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML\n"
        "2.0//EN\">\n"
        "(more data)";

    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_FAILED);
}

static void test_response_invalid_status_code() {
    char text[] = 
        "HTTP/1.1 FOOBAR Forbidden\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5, max=1000\n"
        "Connection: Keep-Alive\n"
        "Age: 3464\n"
        "Date: Wed 10, Aug 2016 09:46:25 GMT\n"
        "X-Cache-Info: caching\n"
        "Content-Length: 220\n"
        "\n"
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML\n"
        "2.0//EN\">\n"
        "(more data)";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_FAILED);
}

static void test_response_custom_status_text() {
    char text[] = 
        "HTTP/1.1 403 FOOBAR\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5 max=1000\n"
        "Connection: Keep-Alive\n"
        "Age: 3464\n"
        "Date: Wed 10 Aug 2016 09:46:25 GMT\n"
        "X-Cache-Info: caching\n"
        "Content-Length: 220\n"
        "\n"
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML\n"
        "2.0//EN\">\n"
        "(more data)";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_SUCCEEDED);
    my_assert(strings_match((string){response.protocol, response.protocol_len}, STR("HTTP/1.1")));
    my_assert(response.status_code == 403);
    my_assert(strings_match((string){response.status_text, response.status_text_len}, STR("FOOBAR")));
    my_assert(response.headers_len == 9);
}

static void test_response_header_with_newline() {
    char text[] = 
        "HTTP/1.1 403 Forbidden\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5 max=1000\n"
        "Connection: Keep-Alive\n"
        "Age: 3464\n"
        "Date: Wed 10 Aug 2016 09:46:25 GMT\n"
        "X-Cache-Info: caching\n"
        "Content-Length: 220\n";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_NOT_ENOUGH_DATA);
}

static void test_response_header_no_newline() {
    char text[] = 
        "HTTP/1.1 403 Forbidden\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5, max=1000\n"
        "Connection: Keep-Alive\n"
        "Age: 3464\n"
        "Date: Wed 10, Aug 2016 09:46:25 GMT\n"
        "X-Cache-Info: caching\n"
        "Content-Length: 220";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_NOT_ENOUGH_DATA);
}

static void test_response_incomplete_header() {
    char text[] = 
        "HTTP/1.1 403 Forbidden\n"
        "Server: Apache\n"
        "Content-Type: text/html; charset=iso-8859-1\n"
        "Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
        "Keep-Alive: timeout-=5, max=1000\n"
        "Conn";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_NOT_ENOUGH_DATA);
}

static void test_response_no_headers() {
    char text[] = 
        "HTTP/1.1 403 Forbidden\n"
        "\n"
        "this one is valid but has no headers";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_SUCCEEDED);
    my_assert(strings_match((string){response.protocol, response.protocol_len}, STR("HTTP/1.1")));
    my_assert(response.status_code == 403);
    my_assert(strings_match((string){response.status_text, response.status_text_len}, STR("Forbidden")));
    my_assert(response.headers_len == 0);
}

static void test_response_only_status_line() {
    char text[] = 
        "HTTP/1.1 403 Forbidden";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_NOT_ENOUGH_DATA);
}

static void test_response_only_incomplete_protocol() {
    char text[] = 
        "HTTP/";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_NOT_ENOUGH_DATA);
}

static void test_response_incomplete_protocol_with_space() {
    char text[] = 
        "HTTP/ ";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_FAILED);
}

static void test_response_incomplete_protocol_with_body() {
    char text[] = 
        "HTTP/\n"
        "\n"
        "body";
    
    http_header_t headers_buf[100];
    http_response_t response;
    http_parsing_result_t result = http_parse_response(text, sizeof(text) - 1,
                                                       headers_buf, ARRAY_LENGTH(headers_buf),
                                                       &response);
    my_assert(result == PARSING_RES_FAILED);
}

int main(int argc, char* argv[]) {
    test_response_full();
    test_response_invalid_protocol();
    test_response_invalid_status_code();
    test_response_custom_status_text();
    test_response_header_with_newline();
    test_response_header_no_newline();
    test_response_incomplete_header();
    test_response_no_headers();
    test_response_only_status_line();
    test_response_only_incomplete_protocol();
    test_response_incomplete_protocol_with_space();
    test_response_incomplete_protocol_with_body();

    printf("All tests passed.\n");
}