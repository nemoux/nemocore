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

#include <nemostring.h>
#include <nemomisc.h>

struct nemostring *nemostring_create(int size)
{
	struct nemostring *str;

	str = (struct nemostring *)malloc(sizeof(struct nemostring));
	if (str == NULL)
		return NULL;

	str->contents = (char *)malloc(size);
	if (str->contents == NULL)
		goto err1;
	str->contents[0] = '\0';
	str->size = size;

	return str;

err1:
	free(str);

	return NULL;
}

void nemostring_destroy(struct nemostring *str)
{
	free(str->contents);
	free(str);
}

void nemostring_append(struct nemostring *str, const char *s)
{
	strcat(str->contents, s);
}

void nemostring_append_one(struct nemostring *str, char c)
{
	strncat(str->contents, &c, 1);
}

void nemostring_append_format(struct nemostring *str, const char *fmt, ...)
{
	va_list vargs;
	char *s;

	va_start(vargs, fmt);
	vasprintf(&s, fmt, vargs);
	va_end(vargs);

	strcat(str->contents, s);

	free(s);
}

void nemostring_tolower(struct nemostring *str)
{
	int length = strlen(str->contents);
	int i;

	for (i = 0; i < length; i++)
		str->contents[i] = tolower(str->contents[i]);
}

void nemostring_toupper(struct nemostring *str)
{
	int length = strlen(str->contents);
	int i;

	for (i = 0; i < length; i++)
		str->contents[i] = toupper(str->contents[i]);
}
