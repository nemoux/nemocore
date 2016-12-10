#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <xmlhelper.h>

#define	XML_NONE_EVENT				(0)
#define	XML_TAG_EVENT					(1)
#define	XML_CHARACTERS_EVENT	(2)

struct xmlparser *xmlparser_create(int max)
{
	struct xmlparser *parser;

	parser = (struct xmlparser *)malloc(sizeof(struct xmlparser));
	if (parser == NULL)
		return NULL;

	parser->buffer = (char *)malloc(max);
	if (parser->buffer == NULL) {
		free(parser);
		return NULL;
	}

	parser->max = max;
	parser->head = 0;
	parser->tail = 0;

	return parser;
}

void xmlparser_destroy(struct xmlparser *parser)
{
	free(parser->buffer);
	free(parser);
}

void xmlparser_push(struct xmlparser *parser, const char *msg, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		parser->buffer[parser->tail] = msg[i];

		parser->tail = (parser->tail + 1) % parser->max;
	}
}

void xmlparser_pop(struct xmlparser *parser, int len)
{
	parser->head = (parser->head + len) % parser->max;
}

int xmlparser_parse(struct xmlparser *parser, char *contents, int *len)
{
	int event = XML_NONE_EVENT;
	int done = 0;
	int o = 0;
	int i;

	for (i = 0; ((parser->head + i) % parser->max) != parser->tail; i++) {
		char ch = parser->buffer[(parser->head + i) % parser->max];

		if (ch == '\0')
			continue;

		if (event == XML_NONE_EVENT) {
			if (ch == ' ' || ch == '\t' || ch == '\n')
				continue;

			if (ch == '<') {
				event = XML_TAG_EVENT;
			} else {
				event = XML_CHARACTERS_EVENT;
			}
		}

		if (event == XML_TAG_EVENT) {
			contents[o++] = ch;

			if (ch == '>') {
				done = 1;
				break;
			}
		}

		if (event == XML_CHARACTERS_EVENT) {
			if (ch == '<') {
				done = 2;
				break;
			}

			contents[o++] = ch;
		}
	}

	if (done == 1) {
		contents[o] = '\0';

		parser->head = (parser->head + i + 1) % parser->max;

		*len = o;
	} else if (done == 2) {
		contents[o] = '\0';

		parser->head = (parser->head + i) % parser->max;

		*len = o;
	} else {
		*len = 0;
	}

	return done;
}

int xmlparser_get_type(const char *contents, int len)
{
	if (contents[0] == '<' && contents[1] == '/')
		return XML_END_TAG_ELEMENT;
	else if (contents[len-2] == '/' && contents[len-1] == '>')
		return XML_STARTEND_TAG_ELEMENT;
	else if (contents[0] == '<')
		return XML_START_TAG_ELEMENT;

	return XML_CHARACTERS_ELEMENT;
}

int xmlparser_get_tag(const char *contents, int len, char *tag)
{
	int i;
	int o = 0;

	for (i = 0; i < len; i++) {
		char ch = contents[i];

		if (ch == '<' || ch == '/')
			continue;

		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '>')
			break;

		tag[o++] = ch;
	}

	tag[o] = '\0';

	return o;
}

int xmlparser_get_attrs(const char *contents, int len, char (*attrs)[XML_TOKEN_SIZE])
{
	char ch;
	int i = 0;
	int n;
	int o;

	// pass tag name
	for (i = 0; i < len; i++) {
		ch = contents[i];

		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '>')
			break;
	}

	for (n = 0; i < len; i++, n++) {
		// get attr name
		for (o = 0; i < len; i++) {	
			ch = contents[i];

			if (ch == '/' || ch == '>')
				goto out;

			if (ch == ' ' || ch == '\t' || ch == '\n')
				continue;

			if (ch == '=')
				break;

			attrs[n*2+0][o++] = ch;
		}

		attrs[n*2+0][o] = '\0';

		for (; i < len; i++) {
			ch = contents[i];

			if (ch == '/' || ch == '>')
				break;

			if (ch == '\"') {
				i++;
				break;
			}
		}

		// get attr value
		for (o = 0; i < len; i++) {
			ch = contents[i];

			if (ch == '\"' || ch == '>') {
				i++;
				break;
			}

			attrs[n*2+1][o++] = ch;
		}

		attrs[n*2+1][o] = '\0';
	}

out:
	return n;
}
