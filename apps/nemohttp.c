#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <http_parser.h>

#include <nemohttp.h>
#include <nemomisc.h>

struct cookiecontext;

typedef void (*nemohttp_cookie_parse_t)(struct cookiecontext *context);

struct cookiecontext {
	struct nemohttp *http;

	const char *contents;
	int icontents;

	char name[NEMOHTTP_COOKIE_NAME_SIZE];
	int iname;
	char value[NEMOHTTP_COOKIE_VALUE_SIZE];
	int ivalue;

	nemohttp_cookie_parse_t *ptable;

	int done;
};

static nemohttp_cookie_parse_t cookie_normal_table[];
static nemohttp_cookie_parse_t cookie_name_table[];
static nemohttp_cookie_parse_t cookie_value_table[];

static inline void nemohttp_cookie_parse_error(struct cookiecontext *context)
{
	context->done = 1;
}

static inline void nemohttp_cookie_parse_ignore(struct cookiecontext *context)
{
	context->icontents++;
}

static inline void nemohttp_cookie_parse_text(struct cookiecontext *context)
{
	context->ptable = cookie_name_table;
	context->name[0] = '\0';
	context->iname = 0;
}

static nemohttp_cookie_parse_t cookie_normal_table[] = {
	[0 ... 255] = nemohttp_cookie_parse_error,
	[' '] = nemohttp_cookie_parse_ignore,
	['\t'] = nemohttp_cookie_parse_ignore,
	['\n'] = nemohttp_cookie_parse_ignore,
	[48 ... 57] = nemohttp_cookie_parse_text,
	[65 ... 90] = nemohttp_cookie_parse_text,
	[97 ... 122] = nemohttp_cookie_parse_text,
};

static inline void nemohttp_cookie_parse_name(struct cookiecontext *context)
{
	context->name[context->iname++] = context->contents[context->icontents++];
	context->name[context->iname] = '\0';
}

static inline void nemohttp_cookie_parse_name_done(struct cookiecontext *context)
{
	context->ptable = cookie_value_table;
	context->value[0] = '\0';
	context->ivalue = 0;
	context->icontents++;
}

static inline void nemohttp_cookie_parse_name_null(struct cookiecontext *context)
{
	context->ptable = cookie_value_table;
	context->value[0] = '\0';
	context->ivalue = 0;
}

static nemohttp_cookie_parse_t cookie_name_table[] = {
	[0 ... 255] = nemohttp_cookie_parse_error,
	[' '] = nemohttp_cookie_parse_name,
	[':'] = nemohttp_cookie_parse_name,
	[';'] = nemohttp_cookie_parse_name,
	[','] = nemohttp_cookie_parse_name,
	['.'] = nemohttp_cookie_parse_name,
	['/'] = nemohttp_cookie_parse_name,
	['-'] = nemohttp_cookie_parse_name,
	['_'] = nemohttp_cookie_parse_name,
	[48 ... 57] = nemohttp_cookie_parse_name,
	[65 ... 90] = nemohttp_cookie_parse_name,
	[97 ... 122] = nemohttp_cookie_parse_name,
	['='] = nemohttp_cookie_parse_name_done,
	['\0'] = nemohttp_cookie_parse_name_null,
};

static inline void nemohttp_cookie_parse_value(struct cookiecontext *context)
{
	context->value[context->ivalue++] = context->contents[context->icontents++];
	context->value[context->ivalue] = '\0';
}

static inline void nemohttp_cookie_parse_value_done(struct cookiecontext *context)
{
	struct nemohttp *http = context->http;

	context->ptable = cookie_normal_table;
	context->icontents++;

	http->cookies[http->ncookies][0] = strdup(context->name);
	http->cookies[http->ncookies][1] = strdup(context->value);

	http->ncookies++;
}

static nemohttp_cookie_parse_t cookie_value_table[] = {
	[0 ... 255] = nemohttp_cookie_parse_error,
	[' '] = nemohttp_cookie_parse_value,
	[':'] = nemohttp_cookie_parse_value,
	[','] = nemohttp_cookie_parse_value,
	['.'] = nemohttp_cookie_parse_value,
	['/'] = nemohttp_cookie_parse_value,
	['-'] = nemohttp_cookie_parse_value,
	['_'] = nemohttp_cookie_parse_value,
	['='] = nemohttp_cookie_parse_value,
	[48 ... 57] = nemohttp_cookie_parse_value,
	[65 ... 90] = nemohttp_cookie_parse_value,
	[97 ... 122] = nemohttp_cookie_parse_value,
	[';'] = nemohttp_cookie_parse_value_done,
	['\0'] = nemohttp_cookie_parse_value_done
};

static int nemohttp_parse_cookie(struct nemohttp *http, const char *contents)
{
	struct cookiecontext _context;
	struct cookiecontext *context = &_context;

	context->http = http;
	context->contents = contents;
	context->icontents = 0;
	context->ptable = cookie_normal_table;
	context->done = 0;

	while (context->done == 0) {
		context->ptable[context->contents[context->icontents]](context);
	}

	return 0;
}

static int nemohttp_handle_message_begin(struct http_parser *parser)
{
	return 0;
}

static int nemohttp_handle_header_field(struct http_parser *parser, const char *buffer, size_t length)
{
	struct nemohttp *http = (struct nemohttp *)parser->data;

	http->fields[http->nfields][0] = strndup(buffer, length);

	return 0;
}

static int nemohttp_handle_header_value(struct http_parser *parser, const char *buffer, size_t length)
{
	struct nemohttp *http = (struct nemohttp *)parser->data;

	http->fields[http->nfields][1] = strndup(buffer, length);

	http->nfields++;

	return 0;
}

static int nemohttp_handle_url(struct http_parser *parser, const char *buffer, size_t length)
{
	struct nemohttp *http = (struct nemohttp *)parser->data;

	http->url = strndup(buffer, length);

	return 0;
}

static int nemohttp_handle_status(struct http_parser *parser, const char *buffer, size_t length)
{
	struct nemohttp *http = (struct nemohttp *)parser->data;

	http->status = strndup(buffer, length);

	return 0;
}

static int nemohttp_handle_body(struct http_parser *parser, const char *buffer, size_t length)
{
	struct nemohttp *http = (struct nemohttp *)parser->data;

	http->body = strdup(buffer);

	return 0;
}

static int nemohttp_handle_headers_complete(struct http_parser *parser)
{
	return 0;
}

static int nemohttp_handle_message_complete(struct http_parser *parser)
{
	return 0;
}

static int nemohttp_handle_chunk_header(struct http_parser *parser)
{
	return 0;
}

static int nemohttp_handle_chunk_complete(struct http_parser *parser)
{
	return 0;
}

static http_parser_settings nemohttp_setting = {
	.on_message_begin = nemohttp_handle_message_begin,
	.on_header_field = nemohttp_handle_header_field,
	.on_header_value = nemohttp_handle_header_value,
	.on_url = nemohttp_handle_url,
	.on_status = nemohttp_handle_status,
	.on_body = nemohttp_handle_body,
	.on_headers_complete = nemohttp_handle_headers_complete,
	.on_message_complete = nemohttp_handle_message_complete,
	.on_chunk_header = nemohttp_handle_chunk_header,
	.on_chunk_complete = nemohttp_handle_chunk_complete
};

struct nemohttp *nemohttp_create(const char *data, int length)
{
	struct http_parser _parser;
	struct http_parser *parser = &_parser;
	struct nemohttp *http;

	http = (struct nemohttp *)malloc(sizeof(struct nemohttp));
	if (http == NULL)
		return NULL;
	memset(http, 0, sizeof(struct nemohttp));

	parser->data = http;

	http_parser_init(parser, HTTP_RESPONSE);

	http_parser_execute(parser, &nemohttp_setting, data, length);

	return http;
}

void nemohttp_destroy(struct nemohttp *http)
{
	int i;

	if (http->status != NULL)
		free(http->status);
	if (http->url != NULL)
		free(http->url);
	if (http->body != NULL)
		free(http->body);

	for (i = 0; i < http->nfields; i++) {
		if (http->fields[i][0] != NULL)
			free(http->fields[i][0]);
		if (http->fields[i][1] != NULL)
			free(http->fields[i][1]);
	}

	for (i = 0; i < http->ncookies; i++) {
		if (http->cookies[i][0] != NULL)
			free(http->cookies[i][0]);
		if (http->cookies[i][1] != NULL)
			free(http->cookies[i][1]);
	}

	free(http);
}

const char *nemohttp_get_status(struct nemohttp *http)
{
	return http->status;
}

const char *nemohttp_get_url(struct nemohttp *http)
{
	return http->url;
}

const char *nemohttp_get_body(struct nemohttp *http)
{
	return http->body;
}

const char *nemohttp_get_field(struct nemohttp *http, const char *name, int *nfield)
{
	int i;

	for (i = *nfield; i < http->nfields; i++) {
		if (strcmp(http->fields[i][0], name) == 0) {
			*nfield = i + 1;

			return http->fields[i][1];
		}
	}

	return NULL;
}

const char *nemohttp_get_cookie(struct nemohttp *http, const char *name)
{
	int i;

	if (http->has_cookies == 0) {
		for (i = 0; i < http->nfields; i++) {
			if (strcmp(http->fields[i][0], "Set-Cookie") == 0) {
				nemohttp_parse_cookie(http, http->fields[i][1]);
			}
		}

		http->has_cookies = 1;
	}

	for (i = 0; i < http->ncookies; i++) {
		if (strcmp(http->cookies[i][0], name) == 0) {
			return http->cookies[i][1];
		}
	}

	return NULL;
}

int nemohttp_needs_follow(struct nemohttp *http)
{
	return http->status != NULL &&
		(strcmp(http->status, "Found") == 0 || strcmp(http->status, "Redirect") == 0);
}

struct nemohttp *nemohttp_follow(struct nemohttp *http)
{
	struct nemohttp *phttp = http;
	const char *data = phttp->body;
	int length = strlen(phttp->body);

	http = nemohttp_create(data, length);

	nemohttp_destroy(phttp);

	return http;
}
