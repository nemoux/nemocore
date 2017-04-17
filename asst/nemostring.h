#ifndef	__NEMO_STRING_H__
#define	__NEMO_STRING_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdlib.h>
#include <string.h>

struct nemostring {
	char *contents;
	int size;
};

extern struct nemostring *nemostring_create(int size);
extern void nemostring_destroy(struct nemostring *str);

extern void nemostring_append(struct nemostring *str, const char *s);
extern void nemostring_append_one(struct nemostring *str, char c);
extern void nemostring_append_format(struct nemostring *str, const char *fmt, ...);

extern void nemostring_tolower(struct nemostring *str);
extern void nemostring_toupper(struct nemostring *str);

static inline const char *nemostring_get_contents(struct nemostring *str)
{
	return str->contents;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
