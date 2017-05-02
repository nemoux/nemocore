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

#include <nemomemo.h>
#include <nemomisc.h>

struct nemomemo *nemomemo_create(int size)
{
	struct nemomemo *memo;

	memo = (struct nemomemo *)malloc(sizeof(struct nemomemo));
	if (memo == NULL)
		return NULL;

	memo->contents = (char *)malloc(size);
	if (memo->contents == NULL)
		goto err1;
	memo->contents[0] = '\0';
	memo->size = size;

	return memo;

err1:
	free(memo);

	return NULL;
}

void nemomemo_destroy(struct nemomemo *memo)
{
	free(memo->contents);
	free(memo);
}

void nemomemo_append(struct nemomemo *memo, const char *s)
{
	strcat(memo->contents, s);
}

void nemomemo_append_one(struct nemomemo *memo, char c)
{
	strncat(memo->contents, &c, 1);
}

void nemomemo_append_format(struct nemomemo *memo, const char *fmt, ...)
{
	va_list vargs;
	char *s;

	va_start(vargs, fmt);
	vasprintf(&s, fmt, vargs);
	va_end(vargs);

	strcat(memo->contents, s);

	free(s);
}

void nemomemo_tolower(struct nemomemo *memo)
{
	int length = strlen(memo->contents);
	int i;

	for (i = 0; i < length; i++)
		memo->contents[i] = tolower(memo->contents[i]);
}

void nemomemo_toupper(struct nemomemo *memo)
{
	int length = strlen(memo->contents);
	int i;

	for (i = 0; i < length; i++)
		memo->contents[i] = toupper(memo->contents[i]);
}
