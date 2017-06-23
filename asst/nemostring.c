#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <regex.h>

#include <nemostring.h>
#include <nemomisc.h>

char *nemostring_append(char *str, const char *s)
{
	char *n;

	if (str == NULL)
		return strdup(s);

	n = (char *)malloc(strlen(str) + strlen(s) + 1);
	if (n == NULL)
		goto out;

	strcpy(n, str);
	strcat(n, s);

	free(str);

out:
	return n;
}

char *nemostring_append_one(char *str, char c)
{
	char *n;

	if (str == NULL)
		return strndup(&c, 1);

	n = (char *)malloc(strlen(str) + 1 + 1);
	if (n == NULL)
		goto out;

	strcpy(n, str);
	strncat(n, &c, 1);

	free(str);

out:
	return n;
}

char *nemostring_append_format(char *str, const char *fmt, ...)
{
	va_list vargs;
	char *s;
	char *n;

	va_start(vargs, fmt);
	vasprintf(&s, fmt, vargs);
	va_end(vargs);

	if (str == NULL)
		return s;

	n = (char *)malloc(strlen(str) + strlen(s) + 1);
	if (n == NULL)
		goto out;

	strcpy(n, str);
	strcat(n, s);

	free(str);

out:
	free(s);

	return n;
}

char *nemostring_replace(const char *str, const char *src, const char *dst)
{
	char *rep;
	char *sub;

	sub = strstr(str, src);
	if (sub == NULL)
		return NULL;

	rep = (char *)malloc(strlen(str) + strlen(dst) - strlen(src) + 1);
	if (rep == NULL)
		return NULL;

	if (str == sub) {
		strcpy(rep, dst);
		strcat(rep, sub + strlen(src));
	} else {
		strncpy(rep, str, sub - str);
		rep[sub - str] = '\0';

		strcat(rep, dst);
		strcat(rep, sub + strlen(src));
	}

	return rep;
}

int nemostring_has_prefix(const char *str, const char *ps)
{
	int length = strlen(ps);
	int i;

	for (i = 0; i < length; i++) {
		if (str[i] != ps[i])
			return 0;
	}

	return 1;
}

int nemostring_has_prefix_format(const char *str, const char *fmt, ...)
{
	va_list vargs;
	char *ps;
	int length;
	int i;

	va_start(vargs, fmt);
	vasprintf(&ps, fmt, vargs);
	va_end(vargs);

	length = strlen(ps);

	for (i = 0; i < length; i++) {
		if (str[i] != ps[i]) {
			free(ps);

			return 0;
		}
	}

	free(ps);

	return 1;
}

int nemostring_has_suffix(const char *str, const char *ss)
{
	int length0 = strlen(str);
	int length1 = strlen(ss);
	int i;

	if (length1 > length0)
		return 0;

	for (i = 1; i <= length1; i++)
		if (str[length0 - i] != ss[length1 - i])
			return 0;

	return 1;
}

int nemostring_has_suffix_format(const char *str, const char *fmt, ...)
{
	va_list vargs;
	char *ss;
	int length0 = strlen(str);
	int length1;
	int i;

	va_start(vargs, fmt);
	vasprintf(&ss, fmt, vargs);
	va_end(vargs);

	length1 = strlen(ss);

	if (length1 > length0)
		goto none;

	for (i = 1; i <= length1; i++)
		if (str[length0 - i] != ss[length1 - i])
			goto none;

	free(ss);

	return 1;

none:
	free(ss);

	return 0;
}

int nemostring_has_regex(const char *str, const char *expr)
{
	regex_t regex;
	int r;

	if (regcomp(&regex, expr, REG_EXTENDED))
		return -1;

	r = regexec(&regex, str, 0, NULL, 0) == 0;

	regfree(&regex);

	return r;
}

int nemostring_has_regex_format(const char *str, const char *fmt, ...)
{
	va_list vargs;
	regex_t regex;
	char *expr;
	int r;

	va_start(vargs, fmt);
	vasprintf(&expr, fmt, vargs);
	va_end(vargs);

	if (regcomp(&regex, expr, REG_EXTENDED)) {
		free(expr);

		return -1;
	}

	r = regexec(&regex, str, 0, NULL, 0) == 0;

	regfree(&regex);

	free(expr);

	return r;
}

int nemostring_parse_decimal(const char *str, int offset, int length)
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

int nemostring_parse_hexadecimal(const char *str, int offset, int length)
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

double nemostring_parse_float(const char *str, int offset, int length)
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

const char *nemostring_find_alphabet(const char *str, int offset, int length)
{
	int i;

	for (i = offset; i < offset + length; i++) {
		if (('a' <= str[i] && str[i] <= 'z') ||
				('A' <= str[i] && str[i] <= 'Z'))
			return &str[i];
	}

	return NULL;
}

const char *nemostring_find_number(const char *str, int offset, int length)
{
	int i;

	for (i = offset; i < offset + length; i++) {
		if ('0' <= str[i] && str[i] <= '9')
			return &str[i];
	}

	return NULL;
}

int nemostring_is_alphabet(const char *str, int offset, int length)
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

int nemostring_is_number(const char *str, int offset, int length)
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
