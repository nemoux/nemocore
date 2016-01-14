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

int nemoshow_sequence_update(struct showone *one)
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

int nemoshow_sequence_update_frame(struct showone *one)
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

int nemoshow_sequence_arrange_set(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src;
	struct showprop *prop;
	struct nemoattr *attr;
	const char *name;
	int i, count;

	src = nemoshow_search_one(show, nemoobject_gets(&one->object, "src"));
	if (src == NULL)
		return -1;
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
				set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
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
				set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->attrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_getd(&one->object, atname);
				set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
				set->dirties[set->nattrs] = prop->dirty;
				set->nattrs++;
			}
		}
	}

	return 0;
}

int nemoshow_sequence_update_set(struct showone *one)
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
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;
		} else if (prop->type == NEMOSHOW_COLOR_PROP) {
			char attr[NEMOSHOW_ATTR_NAME_MAX];

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->dirties[set->nattrs] = prop->dirty;
			set->nattrs++;

			snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
			sattr = nemoobject_get(&src->object, attr);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
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
	set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
	set->dirties[set->nattrs] = dirty;

	return set->nattrs++;
}

int nemoshow_sequence_set_cattr(struct showone *one, const char *name, double r, double g, double b, double a, uint32_t dirty)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	char attr[NEMOSHOW_ATTR_NAME_MAX];
	int index;

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
	set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
	set->dirties[set->nattrs] = dirty;
	index = set->nattrs++;

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
	sattr = nemoobject_get(&src->object, attr);
	set->attrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_getd(&one->object, attr);
	set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
	set->dirties[set->nattrs] = dirty;
	set->nattrs++;

	return index;
}

int nemoshow_sequence_fix_dattr(struct showone *one, int index, double value)
{
	struct showset *set = NEMOSHOW_SET(one);

	set->fattrs[index] = value;

	return 0;
}

static void nemoshow_sequence_prepare_frame(struct showone *one, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	struct showone *child;
	int i;

	nemolist_for_each(child, &one->children_list, children_link) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			set = NEMOSHOW_SET(child);

			for (i = 0; i < set->nattrs; i++) {
				if (nemoattr_get_serial(set->attrs[i]) <= serial) {
					set->sattrs[i] = nemoattr_getd(set->attrs[i]);
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
	struct showone *child;
	double dt = (t - s) / (frame->t - s);
	int i;

	nemolist_for_each(child, &one->children_list, children_link) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(child);

			for (i = 0; i < set->nattrs; i++) {
				if (nemoattr_get_serial(set->attrs[i]) <= serial) {
					nemoattr_setd(set->attrs[i],
							(set->eattrs[i] - set->sattrs[i]) * dt + set->sattrs[i]);

					dirty |= set->dirties[i];
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
	struct showone *child;
	int i;

	nemolist_for_each(child, &one->children_list, children_link) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(child);

			for (i = 0; i < set->nattrs; i++) {
				if (nemoattr_get_serial(set->attrs[i]) <= serial) {
					nemoattr_setd(set->attrs[i], set->fattrs[i]);

					dirty |= set->dirties[i];
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
	struct showone *child;
	int i;

	if (one->serial > serial)
		return;

	one->serial = serial;

	sequence->t = 0.0f;

	sequence->frame = nemolist_node0(&one->children_list, struct showone, children_link);

	nemolist_for_each(frame, &one->children_list, children_link) {
		nemolist_for_each(child, &frame->children_list, children_link) {
			if (child->type == NEMOSHOW_SET_TYPE) {
				struct showset *set = NEMOSHOW_SET(child);

				for (i = 0; i < set->nattrs; i++) {
					if (nemoattr_get_serial(set->attrs[i]) < serial)
						nemoattr_set_serial(set->attrs[i], serial);
				}
			}
		}
	}

	nemoshow_sequence_prepare_frame(sequence->frame, serial);
}

void nemoshow_sequence_dispatch(struct showone *one, double t, uint32_t serial)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);

	if (one->serial > serial)
		return;

	if (sequence->frame != NULL) {
		struct showframe *frame = NEMOSHOW_FRAME(sequence->frame);

		if (t >= 1.0f) {
			nemoshow_sequence_finish_frame(sequence->frame, serial);
		} else if (frame->t < t) {
			nemoshow_sequence_finish_frame(sequence->frame, serial);

			sequence->t = frame->t;

			sequence->frame = nemolist_next(&one->children_list, sequence->frame, struct showone, children_link);
			if (sequence->frame != NULL)
				nemoshow_sequence_prepare_frame(sequence->frame, serial);
		} else {
			nemoshow_sequence_dispatch_frame(sequence->frame, sequence->t, t, serial);
		}
	}
}
