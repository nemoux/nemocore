#ifndef	__STRING_HELPER_H__
#define	__STRING_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern int string_divide(char *str, int length, char div);
extern const char *string_get_token(const char *str, int length, int index);

extern int string_parse_decimal(const char *str, int offset, int length);
extern int string_parse_decimal_with_endptr(const char *str, int offset, int length, const char **endptr);
extern int string_parse_hexadecimal(const char *str, int offset, int length);
extern double string_parse_float(const char *str, int offset, int length);
extern double string_parse_float_with_endptr(const char *str, int offset, int length, const char **endptr);

extern const char *string_find_alphabet(const char *str, int offset, int length);
extern const char *string_find_number(const char *str, int offset, int length);

static inline int string_is_alphabet(const char *str, int offset, int length)
{
	int i;

	for (i = offset; i < offset + length; i++) {
		if (('a' <= str[i] && str[i] <= 'z') ||
				('A' <= str[i] && str[i] <= 'Z'))
			continue;

		return 0;
	}

	return 1;
}

static inline int string_is_number(const char *str, int offset, int length)
{
	int i;

	for (i = offset; i < offset + length; i++) {
		if ('0' <= str[i] && str[i] <= '9')
			continue;

		if (str[i] == '-' || str[i] == '+' || str[i] == '.')
			continue;

		if (str[i] == 'e' || str[i] == 'E')
			continue;

		return 0;
	}

	return 1;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
