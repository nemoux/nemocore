#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showone.h>
#include <showcolor.h>
#include <showmisc.h>
#include <nemobox.h>
#include <nemomisc.h>

void nemoshow_one_prepare(struct showone *one)
{
	nemoobject_prepare(&one->object, NEMOSHOW_ATTR_MAX);

	nemoobject_set_reserved(&one->object, "id", one->id, NEMOSHOW_ID_MAX);

	one->children = (struct showone **)malloc(sizeof(struct showone *) * 4);
	one->nchildren = 0;
	one->schildren = 4;

	one->refs = (struct showone **)malloc(sizeof(struct showone *) * 4);
	one->nrefs = 0;
	one->srefs = 4;

	one->attrs = (struct showattr **)malloc(sizeof(struct showattr *) * 4);
	one->nattrs = 0;
	one->sattrs = 4;

	one->dirty = NEMOSHOW_ALL_DIRTY;
}

void nemoshow_one_finish(struct showone *one)
{
	nemoobject_finish(&one->object);

	free(one->children);
	free(one->refs);

	free(one->attrs);
}

static int nemoshow_one_update_none(struct nemoshow *show, struct showone *one)
{
	return 0;
}

struct showone *nemoshow_one_create(int type)
{
	struct showone *one;

	one = (struct showone *)malloc(sizeof(struct showone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct showone));

	one->type = type;
	one->update = nemoshow_one_update_none;

	nemoshow_one_prepare(one);

	return one;
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

void nemoshow_one_attach_one(struct showone *parent, struct showone *one)
{
	NEMOBOX_APPEND(parent->children, parent->schildren, parent->nchildren, one);

	one->parent = parent;
}

void nemoshow_one_detach_one(struct showone *parent, struct showone *one)
{
	int i;

	for (i = 0; i < parent->nchildren; i++) {
		if (parent->children[i] == one) {
			NEMOBOX_REMOVE(parent->children, parent->nchildren, i);

			break;
		}
	}
}

struct showattr *nemoshow_one_create_attr(const char *name, const char *text, struct nemoattr *ref, uint32_t dirty)
{
	struct showattr *attr;

	attr = (struct showattr *)malloc(sizeof(struct showattr));
	if (attr == NULL)
		return NULL;
	memset(attr, 0, sizeof(struct showattr));

	strcpy(attr->name, name);

	attr->text = strdup(text);
	attr->ref = ref;
	attr->dirty = dirty;

	return attr;
}

void nemoshow_one_destroy_attr(struct showattr *attr)
{
	free(attr->text);
	free(attr);
}

void nemoshow_one_dump(struct showone *one, FILE *out)
{
	struct showprop *prop;
	struct showattr *attr;
	const char *name;
	int i, count;

	fprintf(out, "[%s]\n", one->id);

	count = nemoobject_get_count(&one->object);

	for (i = 0; i < count; i++) {
		name = nemoobject_get_name(&one->object, i);

		prop = nemoshow_get_property(name);
		if (prop != NULL) {
			if (prop->type == NEMOSHOW_DOUBLE_PROP)
				fprintf(out, "  %s = %f\n", name, nemoobject_igetd(&one->object, i));
			else if (prop->type == NEMOSHOW_INTEGER_PROP)
				fprintf(out, "  %s = %d\n", name, nemoobject_igeti(&one->object, i));
			else if (prop->type == NEMOSHOW_STRING_PROP)
				fprintf(out, "  %s = %s\n", name, nemoobject_igets(&one->object, i));
		}
	}

	for (i = 0; i < one->nattrs; i++) {
		attr = one->attrs[i];

		fprintf(out, "  %s = <%s>\n", attr->name, attr->text);
	}
}
