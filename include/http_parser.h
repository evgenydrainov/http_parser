#ifndef LIB_HTTP_PARSER_H
#define LIB_HTTP_PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PARSING_RES_SUCCEEDED,
    PARSING_RES_NOT_ENOUGH_MEMORY,
    PARSING_RES_NOT_ENOUGH_DATA,
    PARSING_RES_FAILED,
} http_parsing_result_t;

static const char* translate_http_parsing_result(http_parsing_result_t result) {
    switch (result) {
        case PARSING_RES_SUCCEEDED:         return "PARSING_RES_SUCCEEDED";
        case PARSING_RES_NOT_ENOUGH_MEMORY: return "PARSING_RES_NOT_ENOUGH_MEMORY";
        case PARSING_RES_NOT_ENOUGH_DATA:   return "PARSING_RES_NOT_ENOUGH_DATA";
        case PARSING_RES_FAILED:            return "PARSING_RES_FAILED";
    }
    return "unknown";
}

/* We don't separate different types of headers (General/Response/Representation) */
typedef struct {
    const char *name;
    size_t name_len;

    const char *value;
    size_t value_len;
} http_header_t;

typedef struct {
    const char *protocol;
    size_t protocol_len;

    uint16_t status_code;

    const char *status_text;
    size_t status_text_len;

    http_header_t *headers;
    size_t headers_len;

    const char *body;
    size_t body_len;
} http_response_t;

/**
 * Parses HTTP response from text buffer to http_response_t structure.
 *
 * @param[in] text, text_len - input buffer with HTTP response text
 * @param[in] headers_buf - pre-allocated array of headers structures that can be used for filling out structure
 * @param[in] headers_max_len - size of headers_buf array
 * @param[out] out_resp - structure that will be filled with data from HTTP response
 *
 * @return error code of parsing
 * @retval PARSING_RES_SUCCEEDED - everything is parsed correctly
 * @retval PARSING_RES_NOT_ENOUGH_MEMORY - headers_max_len is less than actual number of headers in response
 * @retval PARSING_RES_NOT_ENOUGH_DATA - text is correct but not finished HTTP response, user need to provide more data
 * @retval PARSING_RES_FAILED - something went wrong during parsing (for example incorrect input message)
 */
http_parsing_result_t http_parse_response(const char *text, size_t text_len,
                                          http_header_t *headers_buf, size_t headers_max_len,
                                          http_response_t *out_resp);

typedef struct {
    const char *method;
    size_t method_len;

    const char *target;
    size_t target_len;

    const char *protocol;
    size_t protocol_len;

    http_header_t *headers;
    size_t headers_len;

    const char *body;
    size_t body_len;
} http_request_t;

/**
 * Parses HTTP request from text buffer to http_request_t structure.
 *
 * @param[in] text, text_len - input buffer with HTTP request text
 * @param[in] headers_buf - pre-allocated array of headers structures that can be used for filling out structure
 * @param[in] headers_max_len - size of headers_buf array
 * @param[out] out_req - structure that will be filled with data from HTTP request
 *
 * @return error code of parsing
 * @retval PARSING_RES_SUCCEEDED - everything is parsed correctly
 * @retval PARSING_RES_NOT_ENOUGH_MEMORY - headers_max_len is less than actual number of headers in request
 * @retval PARSING_RES_NOT_ENOUGH_DATA - text is correct but not finished HTTP request, user need to provide more data
 * @retval PARSING_RES_FAILED - something went wrong during parsing (for example incorrect input message)
 */
http_parsing_result_t http_parse_request(const char *text, size_t text_len,
                                         http_header_t *headers_buf, size_t headers_max_len,
                                         http_request_t *out_req);


http_parsing_result_t http_decode_chunked(const char* body, size_t body_len,
                                          char* buf, size_t buf_len,
                                          size_t* out_decoded_len);

#endif /* LIB_HTTP_PARSER_H */
