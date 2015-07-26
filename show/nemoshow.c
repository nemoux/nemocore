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

	nemolist_init(&show->ones);

	return show;
}

void nemoshow_destroy(struct nemoshow *show)
{
	nemolist_remove(&show->ones);

	free(show);
}

static struct showone *nemoshow_create_one(struct xmlnode *node)
{
	struct showone *one = NULL;

	if (strcmp(node->name, "scene") == 0) {
		one = nemoshow_scene_create();
	}

	return one;
}

static int nemoshow_load_show(struct nemoshow *show, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(child);
		if (one != NULL) {
			nemoshow_one_parse_xml(one, child);

			nemolist_insert(&show->ones, &one->link);
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

	nemolist_for_each(one, &show->ones, link) {
		if (one->type == NEMOSHOW_SCENE_TYPE) {
			nemoshow_scene_dump(one, out);
		}
	}
}
