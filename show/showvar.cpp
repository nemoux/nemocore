#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showvar.h>
#include <showmisc.h>
#include <nemoshow.h>
#include <nemobox.h>
#include <nemomisc.h>

struct showone *nemoshow_var_create(void)
{
	struct showvar *var;
	struct showone *one;

	var = (struct showvar *)malloc(sizeof(struct showvar));
	if (var == NULL)
		return NULL;
	memset(var, 0, sizeof(struct showvar));

	one = &var->base;
	one->type = NEMOSHOW_VAR_TYPE;
	one->update = nemoshow_var_update;
	one->destroy = nemoshow_var_destroy;

	nemoshow_one_prepare(one);

	return one;
}

void nemoshow_var_destroy(struct showone *one)
{
	struct showvar *var = NEMOSHOW_VAR(one);

	nemoshow_one_finish(one);

	free(var);
}

struct showref *nemoshow_var_create_ref(struct showone *one, const char *name, int type)
{
	struct showref *ref;

	ref = (struct showref *)malloc(sizeof(struct showref));
	if (ref == NULL)
		return NULL;
	memset(ref, 0, sizeof(struct showref));

	ref->attr = nemoobject_get(&one->object, name);
	strcpy(ref->name, name);
	ref->type = type;

	return ref;
}

void nemoshow_var_destroy_ref(struct showref *ref)
{
	free(ref);
}

int nemoshow_var_arrange(struct nemoshow *show, struct showone *one)
{
	return 0;
}

int nemoshow_var_update(struct nemoshow *show, struct showone *one)
{
	struct showvar *var = NEMOSHOW_VAR(one);
	struct showone *child;
	struct showref *ref;
	const char *value;
	int i;

	value = nemoobject_gets(&one->object, "d");

	for (i = 0; i < one->nrefs; i++) {
		child = one->refs[i];
		ref = var->refs[i];

		if (ref->type == NEMOSHOW_STRING_PROP) {
			nemoattr_sets(ref->attr, value, strlen(value));
		} else if (ref->type == NEMOSHOW_DOUBLE_PROP) {
			nemoattr_setd(ref->attr, strtod(value, NULL));
		} else if (ref->type == NEMOSHOW_INTEGER_PROP) {
			nemoattr_seti(ref->attr, strtoul(value, NULL, 10));
		} else if (ref->type == NEMOSHOW_COLOR_PROP) {
			uint32_t c = nemoshow_color_parse(value);
			char attr[NEMOSHOW_ATTR_NAME_MAX];

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", ref->name);
			nemoobject_setd(&child->object, attr, (double)NEMOSHOW_COLOR_UINT32_R(c));
			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", ref->name);
			nemoobject_setd(&child->object, attr, (double)NEMOSHOW_COLOR_UINT32_G(c));
			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", ref->name);
			nemoobject_setd(&child->object, attr, (double)NEMOSHOW_COLOR_UINT32_B(c));
			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", ref->name);
			nemoobject_setd(&child->object, attr, (double)NEMOSHOW_COLOR_UINT32_A(c));

			nemoobject_seti(&child->object, ref->name, 1);
		}
	}

	return 0;
}
