#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemokeys.h>
#include <nemomisc.h>

struct nemokeys *nemokeys_create(int size)
{
	struct nemokeys *keys;

	keys = (struct nemokeys *)malloc(sizeof(struct nemokeys));
	if (keys == NULL)
		return NULL;
	memset(keys, 0, sizeof(struct nemokeys));

	keys->ones = (struct keysone *)malloc(sizeof(struct keysone) * size);
	if (keys->ones == NULL)
		goto err1;

	keys->nones = 0;
	keys->sones = size;

	return keys;

err1:
	free(keys);

	return NULL;
}

void nemokeys_destroy(struct nemokeys *keys)
{
	int i;

	for (i = 0; i < keys->nones; i++) {
		free(keys->ones[i].key);
		free(keys->ones[i].value);
	}

	free(keys->ones);

	free(keys);
}

int nemokeys_set(struct nemokeys *keys, const char *key, const char *value)
{
	int i;

	for (i = 0; i < keys->nones; i++) {
		if (strcmp(keys->ones[i].key, key) == 0) {
			free(keys->ones[i].value);

			keys->ones[i].value = strdup(value);

			return i;
		}
	}

	if (keys->nones >= keys->sones)
		return -1;

	keys->ones[keys->nones].key = strdup(key);
	keys->ones[keys->nones].value = strdup(value);

	return keys->nones++;
}

static int nemokeys_compare_bsearch(const void *a, const void *b)
{
	const struct keysone *one = (const struct keysone *)a;
	const struct keysone *two = (const struct keysone *)b;

	return strcmp(one->key, two->key);
}

const char *nemokeys_get(struct nemokeys *keys, const char *key)
{
	struct keysone *one;

	one = (struct keysone *)bsearch(key,
			keys->ones,
			keys->nones,
			sizeof(struct keysone),
			nemokeys_compare_bsearch);
	if (one == NULL)
		return NULL;

	return one->value;
}

static int nemokeys_compare_qsort(const void *a, const void *b)
{
	const struct keysone *one = (const struct keysone *)a;
	const struct keysone *two = (const struct keysone *)b;

	return strcmp(one->key, two->key);
}

void nemokeys_update(struct nemokeys *keys)
{
	qsort(keys->ones, keys->nones, sizeof(struct keysone), nemokeys_compare_qsort);
}
