#ifndef	__NEMO_STRING_H__
#define	__NEMO_STRING_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdlib.h>
#include <string.h>

extern char *nemostring_append(char *str, const char *s);
extern char *nemostring_append_one(char *str, char c);
extern char *nemostring_append_format(char *str, const char *fmt, ...);

extern char *nemostring_replace(const char *str, const char *src, const char *dst);

extern int nemostring_has_prefix(const char *str, const char *ps);
extern int nemostring_has_prefix_format(const char *str, const char *fmt, ...);
extern int nemostring_has_suffix(const char *str, const char *ss);
extern int nemostring_has_suffix_format(const char *str, const char *fmt, ...);
extern int nemostring_has_regex(const char *str, const char *expr);
extern int nemostring_has_regex_format(const char *str, const char *fmt, ...);

extern int nemostring_parse_decimal(const char *str, int offset, int length);
extern int nemostring_parse_hexadecimal(const char *str, int offset, int length);
extern double nemostring_parse_float(const char *str, int offset, int length);
extern const char *nemostring_find_alphabet(const char *str, int offset, int length);
extern const char *nemostring_find_number(const char *str, int offset, int length);
extern int nemostring_is_alphabet(const char *str, int offset, int length);
extern int nemostring_is_number(const char *str, int offset, int length);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
