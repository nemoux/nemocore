#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <stringhelper.h>

int string_merge(char *str, int length, char div, int count, ...)
{
	va_list vargs;
	const char *t;
	int index = 0;
	int i, j, l;

	va_start(vargs, count);

	for (i = 0; i < count && index < length - 1; i++) {
		t = va_arg(vargs, const char *);
		if (t != NULL) {
			if (index != 0)
				str[index++] = div;

			l = strlen(t);

			for (j = 0; j < l && index < length - 1; j++) {
				str[index++] = t[j];
			}
		}
	}

	str[index] = '\0';

	va_end(vargs);

	return index;
}

int string_parse_decimal(const char *str, int offset, int length)
{
	uint32_t number = 0;
	int sign = 1;
	int has_number = 0;
	int i;

	for (i = offset; i < offset + length; i++) {
		switch (str[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				number = (number * 10) + str[i] - '0';
				has_number = 1;
				break;

			case '-':
				if (has_number != 0)
					goto out;
				sign = -1;
				break;

			default:
				if (has_number != 0)
					goto out;
				break;
		}
	}

out:
	if (has_number != 0)
		return sign * number;

	return 0;
}

int string_parse_decimal_with_endptr(const char *str, int offset, int length, const char **endptr)
{
	uint32_t number = 0;
	int sign = 1;
	int has_number = 0;
	int i;

	for (i = offset; i < offset + length; i++) {
		switch (str[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				number = (number * 10) + str[i] - '0';
				has_number = 1;
				break;

			case '-':
				if (has_number != 0)
					goto out;
				sign = -1;
				break;

			default:
				if (has_number != 0)
					goto out;
				break;
		}
	}

out:
	if (has_number != 0) {
		*endptr = &str[i];

		return sign * number;
	}

	return 0;
}

int string_parse_hexadecimal(const char *str, int offset, int length)
{
	uint32_t number = 0;
	int sign = 1;
	int has_number = 0;
	int i;

	for (i = offset; i < offset + length; i++) {
		switch (str[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				number = (number * 16) + str[i] - '0';
				has_number = 1;
				break;

			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				number = (number * 16) + str[i] - 'a' + 10;
				has_number = 1;
				break;

			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				number = (number * 16) + str[i] - 'A' + 10;
				has_number = 1;
				break;

			case '-':
				if (has_number != 0)
					goto out;
				sign = -1;
				break;

			default:
				if (has_number != 0)
					goto out;
				break;
		}
	}

out:
	if (has_number != 0)
		return sign * number;

	return 0;
}

double string_parse_float(const char *str, int offset, int length)
{
	uint32_t number = 0;
	uint32_t fraction = 0;
	int sign = 1;
	int has_number = 0;
	int has_fraction = 0;
	int fraction_cipher = 1;
	int i;

	for (i = offset; i < offset + length; i++) {
		switch (str[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (has_fraction == 0) {
					number = (number * 10) + str[i] - '0';
				} else {
					fraction = (fraction * 10) + str[i] - '0';
					fraction_cipher = fraction_cipher * 10;
				}
				has_number = 1;
				break;

			case '.':
				has_fraction = 1;
				break;

			case '-':
				if (has_number != 0)
					goto out;
				sign = -1;
				break;

			default:
				if (has_number != 0)
					goto out;
				break;
		}
	}

out:
	if (has_number != 0)
		return sign * ((double)number + (double)fraction / (double)fraction_cipher);

	return 0.0f;
}

double string_parse_float_with_endptr(const char *str, int offset, int length, const char **endptr)
{
	uint32_t number = 0;
	uint32_t fraction = 0;
	int sign = 1;
	int has_number = 0;
	int has_fraction = 0;
	int fraction_cipher = 1;
	int i;

	for (i = offset; i < offset + length; i++) {
		switch (str[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (has_fraction == 0) {
					number = (number * 10) + str[i] - '0';
				} else {
					fraction = (fraction * 10) + str[i] - '0';
					fraction_cipher = fraction_cipher * 10;
				}
				has_number = 1;
				break;

			case '.':
				has_fraction = 1;
				break;

			case '-':
				if (has_number != 0)
					goto out;
				sign = -1;
				break;

			default:
				if (has_number != 0)
					goto out;
				break;
		}
	}

out:
	if (has_number != 0) {
		*endptr = &str[i];

		return sign * ((double)number + (double)fraction / (double)fraction_cipher);
	}

	return 0.0f;
}

const char *string_find_alphabet(const char *str, int offset, int length)
{
	int i;

	for (i = offset; i < offset + length; i++) {
		if (('a' <= str[i] && str[i] <= 'z') ||
				('A' <= str[i] && str[i] <= 'Z'))
			return &str[i];
	}

	return NULL;
}

const char *string_find_number(const char *str, int offset, int length)
{
	int i;

	for (i = offset; i < offset + length; i++) {
		if ('0' <= str[i] && str[i] <= '9')
			return &str[i];
	}

	return NULL;
}

int string_is_alphabet(const char *str, int offset, int length)
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

int string_is_number(const char *str, int offset, int length)
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
