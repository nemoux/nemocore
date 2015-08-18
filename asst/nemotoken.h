#ifndef	__NEMO_TOKEN_H__
#define	__NEMO_TOKEN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemotoken {
	char *contents;
	int length;

	char **tokens;
	int token_count;
};

extern struct nemotoken *nemotoken_create(const char *str, int length);
extern void nemotoken_destroy(struct nemotoken *token);

extern void nemotoken_divide(struct nemotoken *token, char div);
extern int nemotoken_update(struct nemotoken *token);

extern int nemotoken_get_token_count(struct nemotoken *token);
extern char **nemotoken_get_tokens(struct nemotoken *token);
extern char *nemotoken_get_token(struct nemotoken *token, int index);
extern char *nemotoken_get_token_pair(struct nemotoken *token, const char *name);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
