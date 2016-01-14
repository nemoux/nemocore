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
	nemosignal_init(&one->unreference_signal);
	nemosignal_init(&one->dirty_signal);

	nemolist_init(&one->parent_destroy_listener.link);

	nemoobject_set_reserved(&one->object, "id", one->id, NEMOSHOW_ID_MAX);

	one->children = (struct showone **)malloc(sizeof(struct showone *) * 4);
	one->nchildren = 0;
	one->schildren = 4;

	one->attrs = (struct showattr **)malloc(sizeof(struct showattr *) * 4);
	one->nattrs = 0;
	one->sattrs = 4;

	one->dirty = NEMOSHOW_ALL_DIRTY;
}

void nemoshow_one_finish(struct showone *one)
{
	int i;

	nemosignal_emit(&one->destroy_signal, one);
	nemosignal_emit(&one->unreference_signal, one);

	if (one->show != NULL) {
		nemoshow_detach_one(one->show, one);
	}

	if (one->parent != NULL) {
		nemoshow_one_detach_one(one->parent, one);
	}

	for (i = 0; i < one->nattrs; i++) {
		nemoshow_one_destroy_attr(one->attrs[i]);
	}

	nemolist_remove(&one->parent_destroy_listener.link);

	nemoshow_one_unreference_all(one);

	nemoobject_finish(&one->object);

	free(one->children);
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
	if (one->state & NEMOSHOW_RECYCLE_STATE)
		return;

	if (one->destroy != NULL) {
		one->destroy(one);
	} else {
		nemoshow_one_finish(one);

		free(one);
	}
}

void nemoshow_one_destroy_all(struct showone *one)
{
	if (one->state & NEMOSHOW_RECYCLE_STATE)
		return;

	while (one->nchildren > 0)
		nemoshow_one_destroy_all(one->children[0]);

	if (one->destroy != NULL) {
		one->destroy(one);
	} else {
		nemoshow_one_finish(one);

		free(one);
	}
}

static void nemoshow_one_handle_parent_destroy_signal(struct nemolistener *listener, void *data)
{
	struct showone *one = (struct showone *)container_of(listener, struct showone, parent_destroy_listener);

	one->parent = NULL;

	nemolist_remove(&one->parent_destroy_listener.link);
	nemolist_init(&one->parent_destroy_listener.link);
}

void nemoshow_one_attach_one(struct showone *parent, struct showone *one)
{
	NEMOBOX_APPEND(parent->children, parent->schildren, parent->nchildren, one);

	one->parent = parent;

	one->parent_destroy_listener.notify = nemoshow_one_handle_parent_destroy_signal;
	nemosignal_add(&parent->destroy_signal, &one->parent_destroy_listener);
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

	one->parent = NULL;

	nemolist_remove(&one->parent_destroy_listener.link);
	nemolist_init(&one->parent_destroy_listener.link);
}

void nemoshow_one_above_one(struct showone *one, struct showone *above)
{
	struct showone *parent = one->parent;
	int i;

	for (i = 0; i < parent->nchildren; i++) {
		if (parent->children[i] == one) {
			NEMOBOX_REMOVE(parent->children, parent->nchildren, i);

			break;
		}
	}

	for (i = 0; i < parent->nchildren; i++) {
		if (parent->children[i] == above) {
			NEMOBOX_PUSH(parent->children, parent->schildren, parent->nchildren, i + 1, one);

			break;
		}
	}
}

void nemoshow_one_below_one(struct showone *one, struct showone *below)
{
	struct showone *parent = one->parent;
	int i;

	for (i = 0; i < parent->nchildren; i++) {
		if (parent->children[i] == one) {
			NEMOBOX_REMOVE(parent->children, parent->nchildren, i);

			break;
		}
	}

	for (i = 0; i < parent->nchildren; i++) {
		if (parent->children[i] == below) {
			NEMOBOX_PUSH(parent->children, parent->schildren, parent->nchildren, i, one);

			break;
		}
	}
}

static void nemoshow_one_handle_unreference_signal(struct nemolistener *listener, void *data)
{
	struct showref *ref = (struct showref *)container_of(listener, struct showref, unreference_listener);

	ref->one->refs[ref->index] = NULL;
	ref->one->rrefs[ref->index] = NULL;

	nemolist_remove(&ref->unreference_listener.link);

	free(ref);
}

static void nemoshow_one_handle_dirty_signal(struct nemolistener *listener, void *data)
{
	struct showref *ref = (struct showref *)container_of(listener, struct showref, dirty_listener);
	struct showevent *event = (struct showevent *)data;

	nemoshow_one_dirty(ref->one, event->dirty);
}

int nemoshow_one_reference_one(struct showone *one, struct showone *src, int index)
{
	struct showref *ref;

	ref = (struct showref *)malloc(sizeof(struct showref));
	if (ref == NULL)
		return -1;

	ref->one = one;
	ref->index = index;

	ref->unreference_listener.notify = nemoshow_one_handle_unreference_signal;
	nemosignal_add(&src->unreference_signal, &ref->unreference_listener);

	ref->dirty_listener.notify = nemoshow_one_handle_dirty_signal;
	nemosignal_add(&src->dirty_signal, &ref->dirty_listener);

	one->refs[index] = src;
	one->rrefs[index] = ref;

	return 0;
}

void nemoshow_one_unreference_one(struct showone *one, struct showone *src)
{
	int i;

	if (src == NULL)
		return;

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		if (one->refs[i] == src) {
			struct showref *ref = one->rrefs[i];

			one->refs[i] = NULL;
			one->rrefs[i] = NULL;

			nemolist_remove(&ref->unreference_listener.link);
			nemolist_remove(&ref->dirty_listener.link);

			free(ref);

			break;
		}
	}
}

void nemoshow_one_unreference_all(struct showone *one)
{
	struct showone *src;
	struct showref *ref;
	int i, j;

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		one->refs[i] = NULL;

		ref = one->rrefs[i];
		if (ref != NULL) {
			one->rrefs[i] = NULL;

			nemolist_remove(&ref->unreference_listener.link);
			nemolist_remove(&ref->dirty_listener.link);

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

void nemoshow_one_dump(struct showone *one, FILE *out)
{
	struct showprop *prop;
	struct showattr *attr;
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

	for (i = 0; i < one->nchildren; i++) {
		nemoshow_one_dump(one->children[i], out);
	}
}
