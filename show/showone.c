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

struct showattr *nemoshow_one_get_attr(const char *name)
{
	static struct showattr attrs[] = {
		{ "alpha",						NEMOSHOW_DOUBLE_ATTR },
		{ "begin",						NEMOSHOW_INTEGER_ATTR },
		{ "cx",								NEMOSHOW_DOUBLE_ATTR },
		{ "cy",								NEMOSHOW_DOUBLE_ATTR },
		{ "ease",							NEMOSHOW_STRING_ATTR },
		{ "end",							NEMOSHOW_INTEGER_ATTR },
		{ "fill",							NEMOSHOW_COLOR_ATTR },
		{ "font",							NEMOSHOW_STRING_ATTR },
		{ "font-size",				NEMOSHOW_DOUBLE_ATTR },
		{ "from",							NEMOSHOW_DOUBLE_ATTR },
		{ "height",						NEMOSHOW_DOUBLE_ATTR },
		{ "id",								NEMOSHOW_STRING_ATTR },
		{ "r",								NEMOSHOW_DOUBLE_ATTR },
		{ "rx",								NEMOSHOW_DOUBLE_ATTR },
		{ "ry",								NEMOSHOW_DOUBLE_ATTR },
		{ "src",							NEMOSHOW_STRING_ATTR },
		{ "stroke",						NEMOSHOW_COLOR_ATTR },
		{ "stroke-width",			NEMOSHOW_DOUBLE_ATTR },
		{ "t",								NEMOSHOW_DOUBLE_ATTR },
		{ "timing",						NEMOSHOW_STRING_ATTR },
		{ "to",								NEMOSHOW_DOUBLE_ATTR },
		{ "type",							NEMOSHOW_STRING_ATTR },
		{ "width",						NEMOSHOW_DOUBLE_ATTR },
		{ "x",								NEMOSHOW_DOUBLE_ATTR },
		{ "x0",								NEMOSHOW_DOUBLE_ATTR },
		{ "x1",								NEMOSHOW_DOUBLE_ATTR },
		{ "y",								NEMOSHOW_DOUBLE_ATTR },
		{ "y0",								NEMOSHOW_DOUBLE_ATTR },
		{ "y1",								NEMOSHOW_DOUBLE_ATTR },
	};

	return (struct showattr *)bsearch(name,
			attrs,
			sizeof(attrs) / sizeof(attrs[0]),
			sizeof(attrs[0]),
			nemoshow_one_compare_attr);
}

void nemoshow_one_parse_xml(struct showone *one, struct xmlnode *node)
{
	struct showattr *attr;
	int i;

	for (i = 0; i < node->nattrs; i++) {
		attr = nemoshow_one_get_attr(node->attrs[i*2+0]);
		if (attr != NULL) {
			if (attr->type == NEMOSHOW_STRING_ATTR) {
				nemoobject_sets(&one->object, node->attrs[i*2+0], node->attrs[i*2+1], strlen(node->attrs[i*2+1]));
			} else if (attr->type == NEMOSHOW_DOUBLE_ATTR) {
				nemoobject_setd(&one->object, node->attrs[i*2+0], strtod(node->attrs[i*2+1], NULL));
			} else if (attr->type == NEMOSHOW_INTEGER_ATTR) {
				nemoobject_seti(&one->object, node->attrs[i*2+0], strtoul(node->attrs[i*2+1], NULL, 10));
			} else if (attr->type == NEMOSHOW_COLOR_ATTR) {
				nemoobject_seti(&one->object, node->attrs[i*2+0], nemoshow_color_parse(node->attrs[i*2+1]));
			}
		}
	}
}

void nemoshow_one_dump(struct showone *one, FILE *out)
{
	struct showattr *attr;
	const char *name;
	int i, count;

	fprintf(out, "[%s]\n", one->id);

	count = nemoobject_get_count(&one->object);

	for (i = 0; i < count; i++) {
		name = nemoobject_get_name(&one->object, i);

		attr = nemoshow_one_get_attr(name);
		if (attr != NULL) {
			if (attr->type == NEMOSHOW_DOUBLE_ATTR)
				fprintf(out, "  %s = %f\n", name, nemoobject_igetd(&one->object, i));
			else if (attr->type == NEMOSHOW_INTEGER_ATTR)
				fprintf(out, "  %s = %d\n", name, nemoobject_igeti(&one->object, i));
			else if (attr->type == NEMOSHOW_STRING_ATTR)
				fprintf(out, "  %s = %s\n", name, nemoobject_igets(&one->object, i));
			else if (attr->type == NEMOSHOW_COLOR_ATTR)
				fprintf(out, "  %s = 0x%x\n", name, nemoobject_igeti(&one->object, i));
		}
	}
}
