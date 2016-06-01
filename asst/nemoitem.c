#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoitem.h>

struct nemoitem *nemoitem_create(void)
{
	struct nemoitem *item;

	item = (struct nemoitem *)malloc(sizeof(struct nemoitem));
	if (item == NULL)
		return NULL;
	memset(item, 0, sizeof(struct nemoitem));

	nemolist_init(&item->list);

	return item;
}

void nemoitem_destroy(struct nemoitem *item)
{
	struct itemone *one, *next;

	nemolist_for_each_safe(one, next, &item->list, link)
		nemoitem_one_destroy(one);

	free(item);
}

void nemoitem_attach_one(struct nemoitem *item, struct itemone *one)
{
	nemolist_insert_tail(&item->list, &one->link);
}

void nemoitem_detach_one(struct nemoitem *item, struct itemone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);
}

struct itemone *nemoitem_search_one(struct nemoitem *item, const char *path)
{
	struct itemone *one;

	nemolist_for_each(one, &item->list, link) {
		if (strcmp(one->path, path) == 0)
			return one;
	}

	return NULL;
}

struct itemone *nemoitem_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value)
{
	struct itemone *one;

	nemolist_for_each(one, &item->list, link) {
		if ((path == NULL || strcmp(one->path, path) == 0) && nemoitem_one_has_attr(one, name, value) != 0)
			return one;
	}

	return NULL;
}

int nemoitem_count_one(struct nemoitem *item, const char *path)
{
	struct itemone *one;
	int count = 0;

	nemolist_for_each(one, &item->list, link) {
		if (strcmp(one->path, path) == 0)
			count++;
	}

	return count;
}

struct itembox *nemoitem_box_search_one(struct nemoitem *item, const char *path)
{
	struct itembox *box;
	struct itemone *one;
	int count = 0;

	nemolist_for_each(one, &item->list, link) {
		if (strcmp(one->path, path) == 0)
			count++;
	}

	if (count == 0)
		return NULL;

	box = (struct itembox *)malloc(sizeof(struct itembox));
	if (box == NULL)
		return NULL;
	memset(box, 0, sizeof(struct itembox));

	box->ones = (struct itemone **)malloc(sizeof(struct itemone *) * count);
	if (box->ones == NULL)
		goto err1;

	nemolist_for_each(one, &item->list, link) {
		if (strcmp(one->path, path) == 0)
			box->ones[box->nones++] = one;
	}

	return box;

err1:
	free(box);

	return NULL;
}

struct itembox *nemoitem_box_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value)
{
	struct itembox *box;
	struct itemone *one;
	int count = 0;

	nemolist_for_each(one, &item->list, link) {
		if ((path == NULL || strcmp(one->path, path) == 0) && nemoitem_one_has_attr(one, name, value) != 0)
			count++;
	}

	if (count == 0)
		return NULL;

	box = (struct itembox *)malloc(sizeof(struct itembox));
	if (box == NULL)
		return NULL;
	memset(box, 0, sizeof(struct itembox));

	box->ones = (struct itemone **)malloc(sizeof(struct itemone *) * count);
	if (box->ones == NULL)
		goto err1;

	nemolist_for_each(one, &item->list, link) {
		if ((path == NULL || strcmp(one->path, path) == 0) && nemoitem_one_has_attr(one, name, value) != 0)
			box->ones[box->nones++] = one;
	}

	return box;

err1:
	free(box);

	return NULL;
}

void nemoitem_box_destroy(struct itembox *box)
{
	free(box->ones);
	free(box);
}

struct itemone *nemoitem_one_create(void)
{
	struct itemone *one;

	one = (struct itemone *)malloc(sizeof(struct itemone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct itemone));

	nemolist_init(&one->list);

	nemolist_init(&one->link);

	return one;
}

void nemoitem_one_destroy(struct itemone *one)
{
	struct itemattr *attr, *next;

	nemolist_remove(&one->link);

	if (one->path != NULL)
		free(one->path);

	nemolist_for_each_safe(attr, next, &one->list, link) {
		nemolist_remove(&attr->link);

		free(attr->name);
		free(attr->value);
		free(attr);
	}

	free(one);
}

void nemoitem_one_set_path(struct itemone *one, const char *path)
{
	if (one->path != NULL)
		free(one->path);

	one->path = strdup(path);
}

const char *nemoitem_one_get_path(struct itemone *one)
{
	return one->path;
}

int nemoitem_one_has_path(struct itemone *one, const char *path)
{
	return strcmp(one->path, path) == 0;
}

int nemoitem_one_set_attr(struct itemone *one, const char *name, const char *value)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0) {
			free(attr->value);

			attr->value = strdup(value);

			return 1;
		}
	}

	attr = (struct itemattr *)malloc(sizeof(struct itemattr));
	if (attr == NULL)
		return -1;

	attr->name = strdup(name);
	attr->value = strdup(value);

	nemolist_insert_tail(&one->list, &attr->link);

	return 0;
}

const char *nemoitem_one_get_attr(struct itemone *one, const char *name)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0)
			return attr->value;
	}

	return NULL;
}

void nemoitem_one_put_attr(struct itemone *one, const char *name)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0) {
			nemolist_remove(&attr->link);

			free(attr->name);
			free(attr->value);
			free(attr);

			break;
		}
	}
}

int nemoitem_one_has_attr(struct itemone *one, const char *name, const char *value)
{
	struct itemattr *attr;

	nemolist_for_each(attr, &one->list, link) {
		if (strcmp(attr->name, name) == 0) {
			if (value == NULL || strcmp(attr->value, value) == 0)
				return 1;

			break;
		}
	}

	return 0;
}
