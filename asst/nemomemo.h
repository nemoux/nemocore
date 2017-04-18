#ifndef	__NEMO_MEMO_H__
#define	__NEMO_MEMO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdlib.h>
#include <string.h>

struct nemomemo {
	char *contents;
	int size;
};

extern struct nemomemo *nemomemo_create(int size);
extern void nemomemo_destroy(struct nemomemo *memo);

extern void nemomemo_append(struct nemomemo *memo, const char *s);
extern void nemomemo_append_one(struct nemomemo *memo, char c);
extern void nemomemo_append_format(struct nemomemo *memo, const char *fmt, ...);

extern void nemomemo_tolower(struct nemomemo *memo);
extern void nemomemo_toupper(struct nemomemo *memo);

extern int nemomemo_string_parse_decimal(const char *str, int offset, int length);
extern int nemomemo_string_parse_hexadecimal(const char *str, int offset, int length);
extern double nemomemo_string_parse_float(const char *str, int offset, int length);
extern const char *nemomemo_string_find_alphabet(const char *str, int offset, int length);
extern const char *nemomemo_string_find_number(const char *str, int offset, int length);
extern int nemomemo_string_is_alphabet(const char *str, int offset, int length);
extern int nemomemo_string_is_number(const char *str, int offset, int length);

static inline const char *nemomemo_get(struct nemomemo *memo)
{
	return memo->contents;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
