#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stringhelper.h>

int string_divide(char *str, int length, char div)
{
	int i, count, state;

	count = 1;
	state = 1;

	for (i = 0; i < length; i++) {
		if (str[i] == div) {
			if (state != 0)
				count++;

			str[i] = '\0';

			state = 0;
		} else {
			state = 1;
		}
	}

	return count;
}

const char *string_get_token(const char *str, int length, int index)
{
	int i, state;

	state = 1;

	for (i = 0; i < length; i++) {
		if (index <= 0 && state != 0)
			return &str[i];

		if (str[i] == '\0') {
			if (state != 0)
				index--;

			state = 0;
		} else {
			state = 1;
		}
	}

	return NULL;
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
