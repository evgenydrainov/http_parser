#include "http_parser.h"

#include <stdio.h>
#include <assert.h>

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

static void check_http_parse_response(const char* text, size_t text_len,
									  http_parsing_result_t expect_result) {
	http_header_t headers_buf[100];

	http_response_t response;
	http_parsing_result_t result = http_parse_response(text, text_len,
													   headers_buf, ARRAY_LENGTH(headers_buf),
													   &response);

	assert(result == expect_result && "Test failed.");
	if (result != expect_result) {
		printf("Test failed.\n");
		exit(1);
	}

	/*
	printf("Result: %s\n", translate_http_parsing_result(result));

	printf("Protocol: %.*s\n", response.protocol_len, response.protocol);
	printf("Status Code: %u\n", response.status_code);
	printf("Status Text: %.*s\n", response.status_text_len, response.status_text);

	printf("Headers: \n");
	for (size_t i = 0; i < response.headers_len; i++) {
		http_header_t h = response.headers[i];
		printf("%zu: %.*s: %.*s\n", i, h.name_len, h.name, h.value_len, h.value);
	}

	printf("Body: %.*s\n", response.body_len, response.body);
	*/

	printf("Test passed.\n");
}

int main(void) {
	char http_response_full[] =
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
		"(more data)\n";

	char http_response_empty[] = "";

	char http_response_no_body[] =
		"HTTP/1.1 403 Forbidden\n"
		"Server: Apache\n"
		"Content-Type: text/html; charset=iso-8859-1\n"
		"Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
		"Keep-Alive: timeout-=5, max=1000\n"
		"Connection: Keep-Alive\n"
		"Age: 3464\n"
		"Date: Wed 10, Aug 2016 09:46:25 GMT\n"
		"X-Cache-Info: caching\n"
		"Content-Length: 220\n";
	
	char http_response_bad_header[] =
		"HTTP/1.1 403 Forbidden\n"
		"Server: Apache\n"
		"Content-Type: text/html; charset=iso-8859-1\n"
		"Date: Wed, 10 Aug 2016 09:23:25 GMT\n"
		"Keep-Alive: timeout-=5, max=1000\n"
		"Conn";
	
	char http_response_bad_status[] =
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
		"(more data)\n";

	char http_response_only_protocol[] =
		"HTTP/1.1";

	check_http_parse_response(http_response_full,          sizeof(http_response_full)          - 1, PARSING_RES_SUCCEEDED);
	check_http_parse_response(http_response_empty,         sizeof(http_response_empty)         - 1, PARSING_RES_NOT_ENOUGH_DATA);
	check_http_parse_response(http_response_no_body,       sizeof(http_response_no_body)       - 1, PARSING_RES_SUCCEEDED);
	check_http_parse_response(http_response_bad_header,    sizeof(http_response_bad_header)    - 1, PARSING_RES_NOT_ENOUGH_DATA);
	check_http_parse_response(http_response_bad_status,    sizeof(http_response_bad_status)    - 1, PARSING_RES_FAILED);
	check_http_parse_response(http_response_only_protocol, sizeof(http_response_only_protocol) - 1, PARSING_RES_NOT_ENOUGH_DATA);
}
