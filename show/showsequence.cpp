#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showsequence.h>
#include <showitem.h>
#include <showitem.hpp>
#include <nemoshow.h>
#include <showmisc.h>
#include <showhelper.hpp>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_sequence_create(void)
{
	struct showsequence *sequence;
	struct showone *one;

	sequence = (struct showsequence *)malloc(sizeof(struct showsequence));
	if (sequence == NULL)
		return NULL;
	memset(sequence, 0, sizeof(struct showsequence));

	one = &sequence->base;
	one->type = NEMOSHOW_SEQUENCE_TYPE;
	one->update = nemoshow_sequence_update;
	one->destroy = nemoshow_sequence_destroy;

	nemoshow_one_prepare(one);

	return one;
}

void nemoshow_sequence_destroy(struct showone *one)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);

	nemoshow_one_finish(one);

	free(sequence);
}

int nemoshow_sequence_update(struct nemoshow *show, struct showone *one)
{
	return 0;
}

struct showone *nemoshow_sequence_create_frame(void)
{
	struct showframe *frame;
	struct showone *one;

	frame = (struct showframe *)malloc(sizeof(struct showframe));
	if (frame == NULL)
		return NULL;
	memset(frame, 0, sizeof(struct showframe));

	one = &frame->base;
	one->type = NEMOSHOW_FRAME_TYPE;
	one->update = nemoshow_sequence_update_frame;
	one->destroy = nemoshow_sequence_destroy_frame;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "t", &frame->t, sizeof(double));

	return one;
}

void nemoshow_sequence_destroy_frame(struct showone *one)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);

	nemoshow_one_finish(one);

	free(frame);
}

int nemoshow_sequence_update_frame(struct nemoshow *show, struct showone *one)
{
	return 0;
}

int nemoshow_sequence_set_timing(struct showone *one, double t)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);

	frame->t = t;

	return 0;
}

struct showone *nemoshow_sequence_create_set(void)
{
	struct showset *set;
	struct showone *one;

	set = (struct showset *)malloc(sizeof(struct showset));
	if (set == NULL)
		return NULL;
	memset(set, 0, sizeof(struct showset));

	one = &set->base;
	one->type = NEMOSHOW_SET_TYPE;
	one->update = nemoshow_sequence_update_set;
	one->destroy = nemoshow_sequence_destroy_set;

	nemoshow_one_prepare(one);

	return one;
}

void nemoshow_sequence_destroy_set(struct showone *one)
{
	struct showset *set = NEMOSHOW_SET(one);

	nemoshow_one_finish(one);

	free(set);
}

int nemoshow_sequence_arrange_set(struct nemoshow *show, struct showone *one)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src;
	struct showprop *prop;
	struct nemoattr *attr;
	const char *name;
	int i, count;

	src = nemoshow_search_one(show, nemoobject_gets(&one->object, "src"));
	if (src == NULL)
		return -1;
	if ((i = nemoobject_has(&one->object, "child")) >= 0)
		src = nemoshow_one_get_child(src, nemoobject_igeti(&one->object, i));
	set->src = src;

	count = nemoobject_get_count(&one->object);

	for (i = 0; i < count; i++) {
		name = nemoobject_get_name(&one->object, i);

		if (strcmp(name, "id") == 0 || strcmp(name, "src") == 0)
			continue;

		prop = nemoshow_get_property(name);
		if (prop != NULL) {
			if (prop->type == NEMOSHOW_DOUBLE_PROP) {
				attr = nemoobject_get(&src->object, name);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_igetd(&one->object, i);
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;
			} else if (prop->type == NEMOSHOW_COLOR_PROP) {
				char atname[NEMOSHOW_ATTR_NAME_MAX];

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;
			}
		}
	}

	return 0;
}

int nemoshow_sequence_update_set(struct nemoshow *show, struct showone *one)
{
	return 0;
}

int nemoshow_sequence_set_source(struct showone *one, struct showone *src)
{
	struct showset *set = NEMOSHOW_SET(one);

	set->src = src;

	return 0;
}

int nemoshow_sequence_set_attr(struct showone *one, const char *name, const char *value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct showprop *prop;
	struct nemoattr *sattr;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		if (prop->type == NEMOSHOW_DOUBLE_PROP) {
			nemoobject_setd(&one->object, name, strtod(value, NULL));
		} else if (prop->type == NEMOSHOW_INTEGER_PROP) {
			nemoobject_setd(&one->object, name, strtoul(value, NULL, 10));
		} else if (prop->type == NEMOSHOW_COLOR_PROP) {
			uint32_t c = nemoshow_color_parse(value);
			char attr[NEMOSHOW_ATTR_NAME_MAX];

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
			nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_R(c));
			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
			nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_G(c));
			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
			nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_B(c));
			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
			nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_A(c));

			nemoobject_seti(&one->object, name, 1);
		}

		if (prop->type == NEMOSHOW_DOUBLE_PROP) {
			sattr = nemoobject_get(&src->object, name);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, name);
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;
		} else if (prop->type == NEMOSHOW_COLOR_PROP) {
			char attr[NEMOSHOW_ATTR_NAME_MAX];

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;
		}
	}

	return 0;
}

int nemoshow_sequence_set_dattr(struct showone *one, const char *name, double value, uint32_t dirty)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;

	nemoobject_setd(&one->object, name, value);

	sattr = nemoobject_get(&src->object, name);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, name);
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	return 0;
}

int nemoshow_sequence_set_cattr(struct showone *one, const char *name, double r, double g, double b, double a, uint32_t dirty)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	char attr[NEMOSHOW_ATTR_NAME_MAX];

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
	nemoobject_setd(&one->object, attr, r);
	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
	nemoobject_setd(&one->object, attr, g);
	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
	nemoobject_setd(&one->object, attr, b);
	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
	nemoobject_setd(&one->object, attr, a);

	nemoobject_seti(&one->object, name, 1);

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	return 0;
}

static void nemoshow_sequence_prepare_frame(struct showone *one, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	int i, j;

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_SET_TYPE) {
			set = NEMOSHOW_SET(one->children[i]);

			for (j = 0; j < set->nattrs; j++) {
				if (nemoattr_get_serial(set->attrs[j]) <= serial) {
					set->sattrs[j] = nemoattr_getd(set->attrs[j]);
				}
			}

			nemoshow_one_set_state(set->src, NEMOSHOW_TRANSITION_STATE);
		}
	}
}

static void nemoshow_sequence_dispatch_frame(struct showone *one, double s, double t, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	double dt = (t - s) / (frame->t - s);
	int i, j;

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(one->children[i]);

			for (j = 0; j < set->nattrs; j++) {
				if (nemoattr_get_serial(set->attrs[j]) <= serial) {
					nemoattr_setd(set->attrs[j],
							(set->eattrs[j] - set->sattrs[j]) * dt + set->sattrs[j]);

					dirty |= set->dirties[j];
				}
			}

			nemoshow_one_dirty(set->src, dirty);
		}
	}
}

static void nemoshow_sequence_finish_frame(struct showone *one, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	int i, j;

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(one->children[i]);

			for (j = 0; j < set->nattrs; j++) {
				if (nemoattr_get_serial(set->attrs[j]) <= serial) {
					nemoattr_setd(set->attrs[j], set->eattrs[j]);

					dirty |= set->dirties[j];
				}
			}

			nemoshow_one_put_state(set->src, NEMOSHOW_TRANSITION_STATE);

			nemoshow_one_dirty(set->src, dirty);
		}
	}
}

void nemoshow_sequence_prepare(struct showone *one, uint32_t serial)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);
	struct showone *frame;
	int i, j, k;

	if (one->serial > serial)
		return;

	one->serial = serial;

	sequence->t = 0.0f;
	sequence->iframe = 0;

	for (i = 0; i < one->nchildren; i++) {
		frame = one->children[i];

		for (j = 0; j < frame->nchildren; j++) {
			if (frame->children[j]->type == NEMOSHOW_SET_TYPE) {
				struct showset *set = NEMOSHOW_SET(frame->children[j]);

				for (k = 0; k < set->nattrs; k++) {
					if (nemoattr_get_serial(set->attrs[k]) < serial)
						nemoattr_set_serial(set->attrs[k], serial);
				}
			}
		}
	}

	nemoshow_sequence_prepare_frame(one->children[sequence->iframe], serial);
}

void nemoshow_sequence_dispatch(struct showone *one, double t, uint32_t serial)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);

	if (one->serial > serial)
		return;

	if (sequence->iframe < one->nchildren) {
		struct showframe *frame = NEMOSHOW_FRAME(one->children[sequence->iframe]);

		if (t >= 1.0f) {
			nemoshow_sequence_finish_frame(one->children[sequence->iframe], serial);
		} else if (frame->t < t) {
			nemoshow_sequence_finish_frame(one->children[sequence->iframe], serial);

			sequence->t = frame->t;
			sequence->iframe++;

			nemoshow_sequence_prepare_frame(one->children[sequence->iframe], serial);
		} else {
			nemoshow_sequence_dispatch_frame(one->children[sequence->iframe], sequence->t, t, serial);
		}
	}
}
