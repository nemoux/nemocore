#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct nemoshow *nemoshow_create(void)
{
	struct nemoshow *show;

	show = (struct nemoshow *)malloc(sizeof(struct nemoshow));
	if (show == NULL)
		return NULL;
	memset(show, 0, sizeof(struct nemoshow));

	show->ones = (struct showone **)malloc(sizeof(struct showone *) * 8);
	show->nones = 0;
	show->sones = 8;

	return show;
}

void nemoshow_destroy(struct nemoshow *show)
{
	free(show->ones);
	free(show);
}

static int nemoshow_compare_qsort(const void *a, const void *b)
{
	struct showone *oa = *((struct showone **)a);
	struct showone *ob = *((struct showone **)b);

	return strcasecmp(oa->id, ob->id);
}

void nemoshow_update_one(struct nemoshow *show)
{
	ARRAY_QSORT(show->ones, show->nones, nemoshow_compare_qsort);
}

static int nemoshow_compare_bsearch(const void *a, const void *b)
{
	struct showone *ob = *((struct showone **)b);

	return strcasecmp((const char *)a, ob->id);
}

struct showone *nemoshow_search_one(struct nemoshow *show, const char *id)
{
	struct showone **op;

	op = (struct showone **)ARRAY_BSEARCH(show->ones, show->nones, id, nemoshow_compare_bsearch);
	if (op != NULL)
		return *op;

	return NULL;
}

static struct showone *nemoshow_create_one(struct xmlnode *node)
{
	struct showone *one = NULL;

	if (strcmp(node->name, "scene") == 0) {
		one = nemoshow_scene_create();
	} else if (strcmp(node->name, "canvas") == 0) {
		one = nemoshow_canvas_create();
	}

	if (one != NULL) {
		nemoshow_one_parse_xml(one, node);
	}

	return one;
}

static int nemoshow_load_scene(struct nemoshow *show, struct showone *scene, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(child);
		if (one != NULL) {
			ARRAY_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_CANVAS_TYPE) {
			}
		}
	}

	return 0;
}

static int nemoshow_load_show(struct nemoshow *show, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(child);
		if (one != NULL) {
			ARRAY_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_SCENE_TYPE) {
				nemoshow_load_scene(show, one, child);
			}
		}
	}

	return 0;
}

int nemoshow_load_xml(struct nemoshow *show, const char *path)
{
	struct nemoxml *xml;
	struct xmlnode *node;

	xml = nemoxml_create();
	nemoxml_load_file(xml, path);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->children, link) {
		if (strcmp(node->name, "show") == 0)
			nemoshow_load_show(show, node);
	}

	nemoxml_destroy(xml);

	return 0;
}

void nemoshow_dump_all(struct nemoshow *show, FILE *out)
{
	struct showone *one;
	int i;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		if (one->type == NEMOSHOW_SCENE_TYPE) {
			nemoshow_scene_dump(one, out);
		} else if (one->type == NEMOSHOW_CANVAS_TYPE) {
			nemoshow_canvas_dump(one, out);
		}
	}
}
