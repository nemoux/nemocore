#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>
#include <regex.h>

#include <namespacehelper.h>

int namespace_has_prefix(const char *ns, const char *ps)
{
	int length = strlen(ps);
	int i;

	for (i = 0; i < length; i++) {
		if (ns[i] != ps[i])
			return 0;
	}

	return 1;
}

int namespace_has_prefix_format(const char *ns, const char *fmt, ...)
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
		if (ns[i] != ps[i]) {
			free(ps);

			return 0;
		}
	}

	free(ps);

	return 1;
}

int namespace_has_regex(const char *ns, const char *expr)
{
	regex_t regex;
	int r;

	if (regcomp(&regex, expr, REG_EXTENDED))
		return -1;

	r = regexec(&regex, ns, 0, NULL, 0) == 0;

	regfree(&regex);

	return r;
}

int namespace_has_regex_format(const char *ns, const char *fmt, ...)
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

	r = regexec(&regex, ns, 0, NULL, 0) == 0;

	regfree(&regex);

	free(expr);

	return r;
}

int namespace_get_count(const char *ns)
{
	int count = 0;
	int length;
	int i;

	length = strlen(ns);

	for (i = 0; i < length; i++) {
		if (ns[i] == '/')
			count++;
	}

	return count;
}

int namespace_get_path(const char *ns, char *ps, int index)
{
	int count = 0;
	int length;
	int i, j;

	length = strlen(ns);

	for (i = 0; i < length; i++) {
		if (ns[i] == '/')
			count++;

		if (count == index + 1) {
			for (j = 0, i += 1; i < length; i++, j++) {
				if (ns[i] == '/' || ns[i] == '\0')
					break;

				ps[j] = ns[i];
			}

			ps[j] = '\0';

			return 1;
		}
	}

	return 0;
}
