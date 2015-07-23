#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
	token->token_count = 0;

	return token;

err1:
	free(token);

	return NULL;
}

void nemotoken_destroy(struct nemotoken *token)
{
	if (token->tokens != NULL)
		free(token->tokens);

	free(token->contents);
	free(token);
}

void nemotoken_divide(struct nemotoken *token, char div)
{
	int i;

	for (i = 0; i < token->length; i++) {
		if (token->contents[i] == div) {
			token->contents[i] = '\0';
		}
	}
}

int nemotoken_update(struct nemotoken *token)
{
	int i, state, index;

	if (token->tokens != NULL)
		free(token->tokens);

	token->token_count = 0;

	state = 0;

	for (i = 0; i < token->length + 1; i++) {
		if (token->contents[i] == '\0') {
			if (state == 1) {
				token->token_count++;
			}

			state = 0;
		} else {
			state = 1;
		}
	}

	token->tokens = (char **)malloc(sizeof(char *) * (token->token_count + 1));
	if (token->tokens == NULL)
		return -1;
	memset(token->tokens, 0, sizeof(char *) * (token->token_count + 1));

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

	return 0;
}

int nemotoken_get_token_count(struct nemotoken *token)
{
	return token->token_count;
}

char **nemotoken_get_tokens(struct nemotoken *token)
{
	return token->tokens;
}

char *nemotoken_get_token(struct nemotoken *token, int index)
{
	return token->tokens[index];
}

char *nemotoken_get_token_pair(struct nemotoken *token, const char *name)
{
	int i;

	if (token->token_count < 2)
		return NULL;

	for (i = 0; i < token->token_count - 1; i++) {
		if (strcmp(token->tokens[i], name) == 0) {
			return token->tokens[i + 1];
		}
	}

	return NULL;
}
