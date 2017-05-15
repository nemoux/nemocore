#ifndef	__NEMO_TOKEN_H__
#define	__NEMO_TOKEN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdlib.h>
#include <string.h>

#define NEMOTOKEN_TOKEN_MAX			(128)

struct nemotoken {
	char *contents;
	int length;

	char **tokens;
	int ntokens;
};

extern struct nemotoken *nemotoken_create_simple(void);
extern struct nemotoken *nemotoken_create(const char *str, int length);
extern struct nemotoken *nemotoken_create_format(const char *fmt, ...);
extern void nemotoken_destroy(struct nemotoken *token);

extern int nemotoken_append(struct nemotoken *token, const char *str, int length);
extern int nemotoken_append_one(struct nemotoken *token, char c);
extern int nemotoken_append_format(struct nemotoken *token, const char *fmt, ...);

extern void nemotoken_divide(struct nemotoken *token, char div);
extern void nemotoken_replace(struct nemotoken *token, char src, char dst);
extern int nemotoken_update(struct nemotoken *token);

extern char *nemotoken_merge(struct nemotoken *token, char div);

extern void nemotoken_tolower(struct nemotoken *token);
extern void nemotoken_toupper(struct nemotoken *token);

extern int nemotoken_get_index(struct nemotoken *token, const char *name);
extern int nemotoken_has_token(struct nemotoken *token, const char *name);

extern int nemotoken_set_maximum(struct nemotoken *token, int maximum_tokens);

static inline int nemotoken_get_length(struct nemotoken *token)
{
	return token->length;
}

static inline int nemotoken_get_count(struct nemotoken *token)
{
	return token->ntokens;
}

static inline char **nemotoken_get_tokens(struct nemotoken *token)
{
	return token->tokens;
}

static inline const char *nemotoken_get_token(struct nemotoken *token, int index)
{
	return token->tokens[index];
}

static inline const char *nemotoken_get_string(struct nemotoken *token, int index, const char *value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return token->tokens[index];
}

static inline int nemotoken_get_integer(struct nemotoken *token, int index, int value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return strtoul(token->tokens[index], NULL, 10);
}

static inline float nemotoken_get_float(struct nemotoken *token, int index, float value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return strtof(token->tokens[index], NULL);
}

static inline double nemotoken_get_double(struct nemotoken *token, int index, double value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return strtod(token->tokens[index], NULL);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
