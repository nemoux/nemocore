#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showone.h>
#include <nemomisc.h>

void nemoshow_one_prepare(struct showone *one)
{
	nemoobject_prepare(&one->object, NEMOSHOW_ATTR_MAX);

	nemoobject_set_reserved(&one->object, "id", one->id, NEMOSHOW_ID_MAX);

	nemolist_init(&one->link);
}

void nemoshow_one_finish(struct showone *one)
{
	nemoobject_finish(&one->object);

	nemolist_remove(&one->link);
}

void nemoshow_one_destroy(struct showone *one)
{
	if (one->destroy != NULL) {
		one->destroy(one);
	} else {
		nemoshow_one_finish(one);

		free(one);
	}
}

static int nemoshow_one_compare_attr(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

void nemoshow_one_parse_xml(struct showone *one, struct xmlnode *node)
{
	static struct attrparser {
		char name[32];
		int type;
	} parsers[] = {
		{ "height",		NEMOSHOW_DOUBLE_ATTR },
		{ "id",				NEMOSHOW_STRING_ATTR },
		{ "width",		NEMOSHOW_DOUBLE_ATTR },
	}, *parser;
	int i;

	for (i = 0; i < node->nattrs; i++) {
		parser = bsearch(node->attrs[i*2+0], parsers, sizeof(parsers) / sizeof(parsers[0]), sizeof(parsers[0]), nemoshow_one_compare_attr);
		if (parser != NULL) {
			if (parser->type == NEMOSHOW_STRING_ATTR) {
				nemoobject_sets(&one->object, node->attrs[i*2+0], node->attrs[i*2+1], strlen(node->attrs[i*2+1]));
			} else if (parser->type == NEMOSHOW_DOUBLE_ATTR) {
				nemoobject_setd(&one->object, node->attrs[i*2+0], strtod(node->attrs[i*2+1], NULL));
			}
		}
	}
}
