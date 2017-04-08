#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <nemodick.h>

struct nemodick *nemodick_create(void)
{
	struct nemodick *dick;

	dick = (struct nemodick *)malloc(sizeof(struct nemodick));
	if (dick == NULL)
		return NULL;
	memset(dick, 0, sizeof(struct nemodick));

	nemolist_init(&dick->list);

	return dick;
}

void nemodick_destroy(struct nemodick *dick)
{
	struct dickone *one, *next;

	nemolist_for_each_safe(one, next, &dick->list, link) {
		nemolist_remove(&one->link);

		free(one->name);
		free(one);
	}
}

void nemodick_insert(struct nemodick *dick, void *node, const char *name)
{
	struct dickone *one;

	nemolist_for_each(one, &dick->list, link) {
		if (strcmp(one->name, name) == 0) {
			one->node = node;
			return;
		}
	}

	one = (struct dickone *)malloc(sizeof(struct dickone));
	one->name = strdup(name);
	one->node = node;

	nemolist_insert_tail(&dick->list, &one->link);
}

void nemodick_insert_format(struct nemodick *dick, void *node, const char *fmt, ...)
{
	struct dickone *one;
	va_list vargs;
	char *name;

	va_start(vargs, fmt);
	vasprintf(&name, fmt, vargs);
	va_end(vargs);

	nemolist_for_each(one, &dick->list, link) {
		if (strcmp(one->name, name) == 0) {
			one->node = node;

			free(name);

			return;
		}
	}

	one = (struct dickone *)malloc(sizeof(struct dickone));
	one->name = name;
	one->node = node;

	nemolist_insert_tail(&dick->list, &one->link);
}

void nemodick_remove(struct nemodick *dick, const char *name)
{
	struct dickone *one;

	nemolist_for_each(one, &dick->list, link) {
		if (strcmp(one->name, name) == 0) {
			nemolist_remove(&one->link);

			free(one->name);
			free(one);

			return;
		}
	}
}

void nemodick_remove_format(struct nemodick *dick, const char *fmt, ...)
{
	struct dickone *one;
	va_list vargs;
	char *name;

	va_start(vargs, fmt);
	vasprintf(&name, fmt, vargs);
	va_end(vargs);

	nemolist_for_each(one, &dick->list, link) {
		if (strcmp(one->name, name) == 0) {
			nemolist_remove(&one->link);

			free(one->name);
			free(one);

			break;
		}
	}

	free(name);
}

void *nemodick_search(struct nemodick *dick, const char *name)
{
	struct dickone *one;

	nemolist_for_each(one, &dick->list, link) {
		if (strcmp(one->name, name) == 0)
			return one->node;
	}

	return NULL;
}

void *nemodick_search_format(struct nemodick *dick, const char *fmt, ...)
{
	struct dickone *one;
	va_list vargs;
	char *name;

	va_start(vargs, fmt);
	vasprintf(&name, fmt, vargs);
	va_end(vargs);

	nemolist_for_each(one, &dick->list, link) {
		if (strcmp(one->name, name) == 0) {
			free(name);

			return one->node;
		}
	}

	free(name);

	return NULL;
}
