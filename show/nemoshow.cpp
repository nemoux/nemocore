#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <nemoxml.h>
#include <nemobox.h>
#include <nemoattr.h>
#include <nemomisc.h>

struct nemoshow *nemoshow_create(void)
{
	struct nemoshow *show;

	show = (struct nemoshow *)malloc(sizeof(struct nemoshow));
	if (show == NULL)
		return NULL;
	memset(show, 0, sizeof(struct nemoshow));

	show->expr = nemoshow_expr_create();
	if (show->expr == NULL)
		goto err1;

	show->stable = nemoshow_expr_create_symbol();
	if (show->stable == NULL)
		goto err2;

	nemoshow_expr_add_symbol_table(show->expr, show->stable);

	show->ones = (struct showone **)malloc(sizeof(struct showone *) * 8);
	show->nones = 0;
	show->sones = 8;

	return show;

err2:
	nemoshow_expr_destroy(show->expr);

err1:
	free(show);

	return NULL;
}

void nemoshow_destroy(struct nemoshow *show)
{
	nemoshow_expr_destroy(show->expr);
	nemoshow_expr_destroy_symbol(show->stable);

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
	NEMOBOX_QSORT(show->ones, show->nones, nemoshow_compare_qsort);
}

static int nemoshow_compare_bsearch(const void *a, const void *b)
{
	struct showone *ob = *((struct showone **)b);

	return strcasecmp((const char *)a, ob->id);
}

struct showone *nemoshow_search_one(struct nemoshow *show, const char *id)
{
	struct showone **op;

	op = (struct showone **)NEMOBOX_BSEARCH(show->ones, show->nones, id, nemoshow_compare_bsearch);
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
	} else if (strcmp(node->name, "rect") == 0) {
		one = nemoshow_rect_create();
	} else if (strcmp(node->name, "loop") == 0) {
		one = nemoshow_loop_create();
	}

	if (one != NULL) {
		int i;

		for (i = 0; i < node->nattrs; i++) {
			if (node->attrs[i*2+1][0] == '!') {
				struct showattr *attr;

				attr = nemoshow_one_create_attr(
						node->attrs[i*2+0],
						node->attrs[i*2+1] + 1,
						nemoobject_get(&one->object, node->attrs[i*2+0]));

				NEMOBOX_APPEND(one->attrs, one->sattrs, one->nattrs, attr);
			} else {
				struct showprop *prop;

				prop = nemoshow_one_get_property(node->attrs[i*2+0]);
				if (prop != NULL) {
					if (prop->type == NEMOSHOW_STRING_PROP) {
						nemoobject_sets(&one->object, node->attrs[i*2+0], node->attrs[i*2+1], strlen(node->attrs[i*2+1]));
					} else if (prop->type == NEMOSHOW_DOUBLE_PROP) {
						nemoobject_setd(&one->object, node->attrs[i*2+0], strtod(node->attrs[i*2+1], NULL));
					} else if (prop->type == NEMOSHOW_INTEGER_PROP) {
						nemoobject_seti(&one->object, node->attrs[i*2+0], strtoul(node->attrs[i*2+1], NULL, 10));
					} else if (prop->type == NEMOSHOW_COLOR_PROP) {
						nemoobject_seti(&one->object, node->attrs[i*2+0], nemoshow_color_parse(node->attrs[i*2+1]));
					}
				}
			}
		}
	}

	return one;
}

static int nemoshow_load_loop(struct nemoshow *show, struct showone *loop, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);
		}
	}

	return 0;
}

static int nemoshow_load_canvas(struct nemoshow *show, struct showone *canvas, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_LOOP_TYPE) {
				nemoshow_load_loop(show, one, child);
			}
		}
	}

	return 0;
}

static int nemoshow_load_scene(struct nemoshow *show, struct showone *scene, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(child);
		if (one != NULL) {
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

			if (one->type == NEMOSHOW_CANVAS_TYPE) {
				nemoshow_load_canvas(show, one, child);
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
			NEMOBOX_APPEND(show->ones, show->sones, show->nones, one);

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

void nemoshow_update_symbol(struct nemoshow *show, const char *name, double value)
{
	nemoshow_expr_add_symbol(show->stable, name, value);
}

int nemoshow_update_expression(struct nemoshow *show, const char *id, const char *name)
{
	struct showone *one;
	struct showattr *attr;
	double value;
	int i;

	one = nemoshow_search_one(show, id);
	if (one == NULL)
		return 0;

	for (i = 0; i < one->nattrs; i++) {
		attr = one->attrs[i];

		if (strcmp(attr->name, name) == 0) {
			value = nemoshow_expr_dispatch_expression(show->expr, attr->text);

			nemoattr_setd(attr->ref, value);

			return 1;
		}
	}

	return 0;
}

void nemoshow_dump_all(struct nemoshow *show, FILE *out)
{
	struct showone *one;
	int i;

	for (i = 0; i < show->nones; i++) {
		one = show->ones[i];

		nemoshow_one_dump(one, out);
	}
}
