#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <showone.h>
#include <showcolor.h>
#include <showmisc.h>
#include <nemobox.h>
#include <nemomisc.h>

void nemoshow_one_prepare(struct showone *one)
{
	nemoobject_prepare(&one->object, NEMOSHOW_ATTR_MAX);

	nemosignal_init(&one->destroy_signal);

	nemolist_init(&one->link);
	nemolist_init(&one->children_link);
	nemolist_init(&one->dirty_link);
	nemolist_init(&one->bounds_link);

	nemolist_init(&one->children_list);
	nemolist_init(&one->reference_list);

	nemoobject_set_reserved(&one->object, "id", one->id, NEMOSHOW_ID_MAX);

	one->attrs = (struct showattr **)malloc(sizeof(struct showattr *) * 4);
	one->nattrs = 0;
	one->sattrs = 4;
}

void nemoshow_one_finish(struct showone *one)
{
	struct showref *ref, *nref;
	struct showone *child, *nchild;
	int i;

	nemosignal_emit(&one->destroy_signal, one);

	if (one->canvas != NULL) {
		nemoshow_canvas_damage_one(one->canvas, one);
		nemoshow_canvas_detach_one(one);
	}

	nemolist_for_each_safe(ref, nref, &one->reference_list, link) {
		ref->one->refs[ref->index] = NULL;

		nemolist_remove(&ref->link);

		free(ref);
	}

	nemoshow_children_for_each_safe(child, nchild, one) {
		nemoshow_one_destroy(child);
	}

	if (one->show != NULL) {
		nemoshow_detach_one(one);
	}

	if (one->parent != NULL) {
		nemoshow_one_detach(one);
	}

	for (i = 0; i < one->nattrs; i++) {
		nemoshow_one_destroy_attr(one->attrs[i]);
	}

	nemolist_remove(&one->link);
	nemolist_remove(&one->children_link);
	nemolist_remove(&one->dirty_link);
	nemolist_remove(&one->bounds_link);

	nemoshow_one_unreference_all(one);

	nemoobject_finish(&one->object);

	free(one->attrs);
}

static int nemoshow_one_update_none(struct showone *one)
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

static inline void nemoshow_one_dirty_backwards(struct showone *one, uint32_t dirty)
{
	struct nemoshow *show = one->show;
	struct showref *ref;

	if (show == NULL)
		return;

	one->dirty |= dirty;

	one->dirty_serial = ++show->dirty_serial;

	nemolist_remove(&one->dirty_link);
	nemolist_insert_tail(&show->dirty_list, &one->dirty_link);

	nemolist_for_each(ref, &one->reference_list, link) {
		if (ref->one->dirty_serial <= one->dirty_serial)
			nemoshow_one_dirty_backwards(ref->one, ref->dirty);
	}

	if (nemoshow_one_has_state(one, NEMOSHOW_EFFECT_STATE)) {
		nemoshow_one_dirty_backwards(one->parent, one->effect);
	}

	if (nemoshow_one_has_state(one, NEMOSHOW_INHERIT_STATE)) {
		struct showone *child;

		nemoshow_children_for_each(child, one) {
			if (nemoshow_one_has_state(child, NEMOSHOW_EFFECT_STATE) == 0)
				nemoshow_one_dirty_backwards(child, dirty);
		}
	}
}

void nemoshow_one_dirty(struct showone *one, uint32_t dirty)
{
	struct nemoshow *show = one->show;
	struct showref *ref;

	if (show == NULL || dirty == 0x0 || (one->dirty & dirty) == dirty)
		return;

	one->dirty |= dirty;

	if (one->dirty_serial == 0) {
		one->dirty_serial = ++show->dirty_serial;

		nemolist_insert_tail(&show->dirty_list, &one->dirty_link);
	}

	nemolist_for_each(ref, &one->reference_list, link) {
		if (ref->one->dirty_serial <= one->dirty_serial)
			nemoshow_one_dirty_backwards(ref->one, ref->dirty);
	}

	if (nemoshow_one_has_state(one, NEMOSHOW_EFFECT_STATE)) {
		nemoshow_one_dirty_backwards(one->parent, one->effect);
	}

	if (nemoshow_one_has_state(one, NEMOSHOW_INHERIT_STATE)) {
		struct showone *child;

		nemoshow_children_for_each(child, one) {
			if (nemoshow_one_has_state(child, NEMOSHOW_EFFECT_STATE) == 0)
				nemoshow_one_dirty_backwards(child, dirty);
		}
	}
}

void nemoshow_one_bounds(struct showone *one)
{
	struct nemoshow *show = one->show;

	if (show == NULL)
		return;

	if (nemoshow_one_has_state(one->parent, NEMOSHOW_BOUNDS_STATE)) {
		nemolist_remove(&one->parent->bounds_link);
		nemolist_insert_tail(&show->bounds_list, &one->parent->bounds_link);

		nemoshow_one_bounds(one->parent);
	}
}

void nemoshow_one_update_bounds(struct showone *one)
{
	if (nemoshow_children_empty(one) != 0) {
		one->x0 = 0;
		one->y0 = 0;
		one->x1 = 0;
		one->y1 = 0;

		one->x = 0;
		one->y = 0;
		one->w = 0;
		one->h = 0;

		one->sx = 0;
		one->sy = 0;
		one->sw = 0;
		one->sh = 0;
	} else {
		struct showone *child;
		int32_t x00, y00, x01, y01, x10, y10, x11, y11, x20, y20, x21, y21;

		x00 = INT_MAX;
		y00 = INT_MAX;
		x01 = INT_MIN;
		y01 = INT_MIN;

		x10 = INT_MAX;
		y10 = INT_MAX;
		x11 = INT_MIN;
		y11 = INT_MIN;

		x20 = INT_MAX;
		y20 = INT_MAX;
		x21 = INT_MIN;
		y21 = INT_MIN;

		nemoshow_children_for_each(child, one) {
			if (child->x0 < x00)
				x00 = child->x0;
			if (child->y0 < y00)
				y00 = child->y0;
			if (child->x1 > x01)
				x01 = child->x1;
			if (child->y1 > y01)
				y01 = child->y1;

			if (child->x < x10)
				x10 = child->x;
			if (child->y < y10)
				y10 = child->y;
			if (child->x + child->w > x11)
				x11 = child->x + child->w;
			if (child->y + child->h > y11)
				y11 = child->y + child->h;

			if (child->sx < x20)
				x20 = child->sx;
			if (child->sy < y20)
				y20 = child->sy;
			if (child->sx + child->sw > x21)
				x21 = child->sx + child->sw;
			if (child->sy + child->sh > y21)
				y21 = child->sy + child->sh;
		}

		one->x0 = x00;
		one->y0 = y00;
		one->x1 = x01;
		one->y1 = y01;

		one->x = x10;
		one->y = y10;
		one->w = x11 - x10;
		one->h = y11 - y10;

		one->sx = x20;
		one->sy = y20;
		one->sw = x21 - x20;
		one->sh = y21 - y20;
	}

	nemolist_remove(&one->bounds_link);
	nemolist_init(&one->bounds_link);
}

void nemoshow_one_attach_one(struct showone *parent, struct showone *one)
{
	if (one->parent != NULL)
		nemoshow_one_detach(one);

	nemolist_insert_tail(&parent->children_list, &one->children_link);
	one->parent = parent;

	if (parent->show != NULL)
		nemoshow_attach_ones(parent->show, one);
}

void nemoshow_one_detach_one(struct showone *one)
{
	nemolist_remove(&one->children_link);
	nemolist_init(&one->children_link);

	nemolist_remove(&one->dirty_link);
	nemolist_init(&one->dirty_link);

	one->parent = NULL;
}

int nemoshow_one_above_one(struct showone *one, struct showone *above)
{
	nemolist_remove(&one->children_link);

	if (above != NULL)
		nemolist_insert_tail(&above->children_link, &one->children_link);
	else
		nemolist_insert_tail(&one->parent->children_list, &one->children_link);

	return 0;
}

int nemoshow_one_below_one(struct showone *one, struct showone *below)
{
	nemolist_remove(&one->children_link);

	if (below != NULL)
		nemolist_insert(&below->children_link, &one->children_link);
	else
		nemolist_insert(&one->parent->children_list, &one->children_link);

	return 0;
}

int nemoshow_one_reference_one(struct showone *one, struct showone *src, uint32_t dirty, int index)
{
	struct showref *ref;

	ref = (struct showref *)malloc(sizeof(struct showref));
	if (ref == NULL)
		return -1;

	ref->src = src;
	ref->dirty = dirty;
	ref->one = one;
	ref->index = index;

	one->refs[index] = ref;

	nemolist_insert(&src->reference_list, &ref->link);

	nemoshow_one_dirty(one, dirty);

	if (one->dirty_serial <= src->dirty_serial)
		nemoshow_one_dirty_backwards(one, dirty);

	if (one->show != NULL)
		nemoshow_attach_ones(one->show, src);

	return 0;
}

void nemoshow_one_unreference_one(struct showone *one, struct showone *src)
{
	int i;

	if (src == NULL)
		return;

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		struct showref *ref = one->refs[i];

		if (ref != NULL && ref->src == src) {
			one->refs[i] = NULL;

			nemolist_remove(&ref->link);

			free(ref);

			break;
		}
	}
}

void nemoshow_one_unreference_all(struct showone *one)
{
	struct showref *ref;
	int i;

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		ref = one->refs[i];
		if (ref != NULL) {
			one->refs[i] = NULL;

			nemolist_remove(&ref->link);

			free(ref);
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

struct showone *nemoshow_one_search_id(struct showone *one, const char *id)
{
	struct showone *child;
	struct showone *sone;

	if (strcmp(one->id, id) == 0)
		return one;

	nemoshow_children_for_each(child, one) {
		sone = nemoshow_one_search_id(child, id);
		if (sone != NULL)
			return sone;
	}

	return NULL;
}

struct showone *nemoshow_one_search_tag(struct showone *one, uint32_t tag)
{
	struct showone *child;
	struct showone *sone;

	if (one->tag == tag)
		return one;

	nemoshow_children_for_each(child, one) {
		sone = nemoshow_one_search_tag(child, tag);
		if (sone != NULL)
			return sone;
	}

	return NULL;
}

void nemoshow_one_dump(struct showone *one, FILE *out)
{
	struct showprop *prop;
	struct showattr *attr;
	struct showone *child;
	const char *name;
	int i, count;

	fprintf(out, "[%s] %u\n", one->id, one->tag);

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

	nemoshow_children_for_each(child, one)
		nemoshow_one_dump(child, out);
}
