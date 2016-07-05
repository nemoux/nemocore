#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>

#include <nemotoken.h>

struct nemotoken *nemotoken_create(const char *str, int length)
{
	struct nemotoken *token;

	token = (struct nemotoken *)malloc(sizeof(struct nemotoken));
	if (token == NULL)
		return NULL;
	memset(token, 0, sizeof(struct nemotoken));

	token->contents = (char *)malloc(length + 1);
	if (token->contents == NULL)
		goto err1;
	memcpy(token->contents, str, length);

	token->contents[length] = '\0';

	token->length = length;
	token->tokens = NULL;
	token->ntokens = 0;

	return token;

err1:
	free(token);

	return NULL;
}

struct nemotoken *nemotoken_create_format(const char *fmt, ...)
{
	struct nemotoken *token;
	va_list vargs;

	token = (struct nemotoken *)malloc(sizeof(struct nemotoken));
	if (token == NULL)
		return NULL;
	memset(token, 0, sizeof(struct nemotoken));

	va_start(vargs, fmt);
	vasprintf(&token->contents, fmt, vargs);
	va_end(vargs);

	token->length = strlen(token->contents);
	token->tokens = NULL;
	token->ntokens = 0;

	return token;
}

void nemotoken_destroy(struct nemotoken *token)
{
	if (token->tokens != NULL)
		free(token->tokens);

	free(token->contents);
	free(token);
}

int nemotoken_append(struct nemotoken *token, const char *str, int length)
{
	char *contents;

	contents = (char *)malloc(token->length + length + 1);
	if (contents == NULL)
		return -1;

	strcpy(contents, token->contents);
	strcat(contents, str);
	contents[token->length + length] = '\0';

	free(token->contents);

	token->contents = contents;
	token->length = token->length + length;

	return 0;
}

int nemotoken_append_one(struct nemotoken *token, char c)
{
	char *contents;

	contents = (char *)malloc(token->length + 1 + 1);
	if (contents == NULL)
		return -1;

	snprintf(contents, token->length + 1 + 1, "%s%c", token->contents, c);

	free(token->contents);

	token->contents = contents;
	token->length = token->length + 1;

	return 0;
}

int nemotoken_append_format(struct nemotoken *token, const char *fmt, ...)
{
	va_list vargs;
	char *contents;
	char *str;
	int length;

	va_start(vargs, fmt);
	vasprintf(&str, fmt, vargs);
	va_end(vargs);

	length = strlen(str);

	contents = (char *)malloc(token->length + length + 1);
	if (contents == NULL)
		return -1;

	strcpy(contents, token->contents);
	strcat(contents, str);
	contents[token->length + length] = '\0';

	free(token->contents);
	free(str);

	token->contents = contents;
	token->length = token->length + length;

	return 0;
}

void nemotoken_fence(struct nemotoken *token, char fence)
{
	token->fence = fence;
}

void nemotoken_divide(struct nemotoken *token, char div)
{
	int i;

	if (token->fence != '\0') {
		int in_fence = 0;

		for (i = 0; i < token->length; i++) {
			if (in_fence == 0) {
				if (token->contents[i] == div) {
					token->contents[i] = '\0';
				} else if (token->contents[i] == token->fence && (i == 0 || token->contents[i - 1] == '\0')) {
					in_fence = 1;
				}
			} else {
				if (token->contents[i] == token->fence && (i + 1 == token->length || token->contents[i + 1] == div)) {
					in_fence = 0;
				}
			}
		}
	} else {
		for (i = 0; i < token->length; i++) {
			if (token->contents[i] == div)
				token->contents[i] = '\0';
		}
	}
}

void nemotoken_replace(struct nemotoken *token, char src, char dst)
{
	int i;

	for (i = 0; i < token->length; i++) {
		if (token->contents[i] == src) {
			token->contents[i] = dst;
		}
	}
}

int nemotoken_update(struct nemotoken *token)
{
	int i, state, index;

	if (token->tokens != NULL)
		free(token->tokens);

	if (token->fence != '\0') {
		int in_fence = 0;

		for (i = 0; i < token->length; i++) {
			if (in_fence == 0) {
				if ((token->contents[i] == token->fence) && (i == 0 || token->contents[i - 1] == '\0')) {
					token->contents[i] = '\0';

					in_fence = 1;
				}
			} else {
				if ((token->contents[i] == token->fence) && (i + 1 == token->length || token->contents[i + 1] == '\0')) {
					token->contents[i] = '\0';

					in_fence = 0;
				}
			}
		}
	}

	token->ntokens = 0;

	state = 0;

	for (i = 0; i < token->length + 1; i++) {
		if (token->contents[i] == '\0') {
			if (state == 1) {
				token->ntokens++;
			}

			state = 0;
		} else {
			state = 1;
		}
	}

	token->tokens = (char **)malloc(sizeof(char *) * (token->ntokens + 1));
	if (token->tokens == NULL)
		return -1;
	memset(token->tokens, 0, sizeof(char *) * (token->ntokens + 1));

	state = 0;
	index = 0;

	for (i = 0; i < token->length + 1; i++) {
		if (token->contents[i] == '\0') {
			state = 0;
		} else {
			if (state == 0) {
				token->tokens[index++] = &token->contents[i];
			}

			state = 1;
		}
	}

	token->tokens[index] = NULL;

	return 0;
}

void nemotoken_tolower(struct nemotoken *token)
{
	int i;

	for (i = 0; i < token->length; i++)
		token->contents[i] = tolower(token->contents[i]);
}

void nemotoken_toupper(struct nemotoken *token)
{
	int i;

	for (i = 0; i < token->length; i++)
		token->contents[i] = toupper(token->contents[i]);
}

int nemotoken_get_index(struct nemotoken *token, const char *name)
{
	int i;

	for (i = 0; i < token->ntokens; i++) {
		if (strcmp(token->tokens[i], name) == 0)
			return i;
	}

	return token->ntokens;
}

int nemotoken_has_token(struct nemotoken *token, const char *name)
{
	int i;

	for (i = 0; i < token->ntokens; i++) {
		if (strcmp(token->tokens[i], name) == 0)
			return 1;
	}

	return 0;
}
