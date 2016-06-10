#ifndef	__STRING_HELPER_H__
#define	__STRING_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern int string_divide(char *str, int length, char div);
extern void string_replace(char *str, int length, char src, char dst);

extern int string_parse_decimal(const char *str, int offset, int length);
extern int string_parse_decimal_with_endptr(const char *str, int offset, int length, const char **endptr);
extern int string_parse_hexadecimal(const char *str, int offset, int length);
extern double string_parse_float(const char *str, int offset, int length);
extern double string_parse_float_with_endptr(const char *str, int offset, int length, const char **endptr);

extern const char *string_find_alphabet(const char *str, int offset, int length);
extern const char *string_find_number(const char *str, int offset, int length);

extern int string_is_alphabet(const char *str, int offset, int length);
extern int string_is_number(const char *str, int offset, int length);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
