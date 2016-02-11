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

struct nemoitem *nemoitem_create(int mnodes)
{
	struct nemoitem *item;

	item = (struct nemoitem *)malloc(sizeof(struct nemoitem));
	if (item == NULL)
		return NULL;
	memset(item, 0, sizeof(struct nemoitem));

	item->nodes = (struct itemnode *)malloc(sizeof(struct itemnode) * mnodes);
	if (item->nodes == NULL)
		goto err1;
	item->nnodes = 0;
	item->mnodes = mnodes;

	return item;

err1:
	free(item);

	return NULL;
}

void nemoitem_destroy(struct nemoitem *item)
{
	int i, j;

	for (i = 0; i < item->nnodes; i++) {
		struct itemnode *node = &item->nodes[i];

		for (j = 0; j < node->nattrs; j++) {
			free(node->attrs[j*2+0]);
			free(node->attrs[j*2+1]);
		}

		free(node->name);
	}

	free(item->nodes);
	free(item);
}

int nemoitem_set(struct nemoitem *item, const char *name)
{
	struct itemnode *node;

	if (item->nnodes >= item->mnodes)
		return -1;

	node = &item->nodes[item->nnodes];
	node->name = strdup(name);
	node->nattrs = 0;

	return item->nnodes++;
}

int nemoitem_get(struct nemoitem *item, const char *name, int index)
{
	struct itemnode *node;
	int i;

	if (index >= item->mnodes)
		return -1;

	for (i = index; i < item->nnodes; i++) {
		node = &item->nodes[i];

		if (strcmp(node->name, name) == 0)
			return i;
	}

	return -1;
}

int nemoitem_get_ifone(struct nemoitem *item, const char *name, int index, const char *attr0, const char *value0)
{
	struct itemnode *node;
	int i, j;

	if (index >= item->mnodes)
		return -1;

	for (i = index; i < item->nnodes; i++) {
		node = &item->nodes[i];

		if (strcmp(node->name, name) == 0) {
			for (j = 0; j < node->nattrs; j++) {
				if (strcmp(node->attrs[j*2+0], attr0) == 0 &&
						strcmp(node->attrs[j*2+1], value0) == 0) {
					return i;
				}
			}
		}
	}

	return -1;
}

int nemoitem_get_iftwo(struct nemoitem *item, const char *name, int index, const char *attr0, const char *value0, const char *attr1, const char *value1)
{
	struct itemnode *node;
	int i, j, count;

	if (index >= item->mnodes)
		return -1;

	for (i = index; i < item->nnodes; i++) {
		node = &item->nodes[i];

		if (strcmp(node->name, name) == 0) {
			count = 0;

			for (j = 0; j < node->nattrs; j++) {
				if (strcmp(node->attrs[j*2+0], attr0) == 0 &&
						strcmp(node->attrs[j*2+1], value0) == 0) {
					count++;
				} else if (strcmp(node->attrs[j*2+0], attr1) == 0 &&
						strcmp(node->attrs[j*2+1], value1) == 0) {
					count++;
				}
			}

			if (count >= 2)
				return i;
		}
	}

	return -1;
}

int nemoitem_set_attr(struct nemoitem *item, int index, const char *attr, const char *value)
{
	struct itemnode *node;

	if (index >= item->mnodes || value == NULL)
		return -1;

	node = &item->nodes[index];
	node->attrs[node->nattrs * 2 + 0] = strdup(attr);
	node->attrs[node->nattrs * 2 + 1] = strdup(value);

	return node->nattrs++;
}

char *nemoitem_get_attr(struct nemoitem *item, int index, const char *attr)
{
	struct itemnode *node;
	int i;

	if (index >= item->mnodes)
		return NULL;

	node = &item->nodes[index];

	for (i = 0; i < node->nattrs; i++) {
		if (strcmp(node->attrs[i*2+0], attr) == 0) {
			return node->attrs[i*2+1];
		}
	}

	return NULL;
}

int nemoitem_get_iattr(struct nemoitem *item, int index, const char *attr, int value)
{
	struct itemnode *node;
	int i;

	if (index >= item->mnodes)
		return value;

	node = &item->nodes[index];

	for (i = 0; i < node->nattrs; i++) {
		if (strcmp(node->attrs[i*2+0], attr) == 0) {
			return strtoul(node->attrs[i*2+1], NULL, 10);
		}
	}

	return value;
}

float nemoitem_get_fattr(struct nemoitem *item, int index, const char *attr, float value)
{
	struct itemnode *node;
	int i;

	if (index >= item->mnodes)
		return value;

	node = &item->nodes[index];

	for (i = 0; i < node->nattrs; i++) {
		if (strcmp(node->attrs[i*2+0], attr) == 0) {
			return strtod(node->attrs[i*2+1], NULL);
		}
	}

	return value;
}

char *nemoitem_get_vattr(struct nemoitem *item, int index, const char *fmt, ...)
{
	struct itemnode *node;
	va_list vargs;
	char *attr;
	int i;

	if (index >= item->mnodes)
		return NULL;

	node = &item->nodes[index];

	va_start(vargs, fmt);
	vasprintf(&attr, fmt, vargs);
	va_end(vargs);

	for (i = 0; i < node->nattrs; i++) {
		if (strcmp(node->attrs[i*2+0], attr) == 0) {
			free(attr);

			return node->attrs[i*2+1];
		}
	}

	free(attr);

	return NULL;
}

void nemoitem_dump(struct nemoitem *item, FILE *out)
{
	struct itemnode *node;
	int i, j;

	for (i = 0; i < item->nnodes; i++) {
		node = &item->nodes[i];

		fprintf(out, "[NEMOITEM] node '%s'\n", node->name);

		for (j = 0; j < node->nattrs; j++) {
			fprintf(out, "[NEMOITEM]   %s = '%s'\n", node->attrs[j*2+0], node->attrs[j*2+1]);
		}
	}
}
