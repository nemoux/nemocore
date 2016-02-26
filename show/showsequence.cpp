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
			double d = strtod(value, NULL);

			sattr = nemoobject_get(&src->object, name);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = d;
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->dirties[set->nattrs] = prop->dirty;
			set->states[set->nattrs] = prop->state;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;

			return set->nattrs++;
		} else if (prop->type == NEMOSHOW_FLOAT_PROP) {
			double d = strtod(value, NULL);

			sattr = nemoobject_get(&src->object, name);
			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = d;
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->dirties[set->nattrs] = prop->dirty;
			set->states[set->nattrs] = prop->state;
			set->types[set->nattrs] = NEMOSHOW_FLOAT_PROP;

			return set->nattrs++;
		} else if (prop->type == NEMOSHOW_COLOR_PROP) {
			uint32_t c = nemoshow_color_parse(value);

			sattr = nemoobject_get(&src->object, name);

			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = (double)NEMOSHOW_COLOR_UINT32_R(c);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->offsets[set->nattrs] = NEMOSHOW_ITEM_RED_COLOR;
			set->dirties[set->nattrs] = prop->dirty;
			set->states[set->nattrs] = prop->state;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = (double)NEMOSHOW_COLOR_UINT32_G(c);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->offsets[set->nattrs] = NEMOSHOW_ITEM_GREEN_COLOR;
			set->dirties[set->nattrs] = prop->dirty;
			set->states[set->nattrs] = prop->state;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = (double)NEMOSHOW_COLOR_UINT32_B(c);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->offsets[set->nattrs] = NEMOSHOW_ITEM_BLUE_COLOR;
			set->dirties[set->nattrs] = prop->dirty;
			set->states[set->nattrs] = prop->state;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			set->attrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = (double)NEMOSHOW_COLOR_UINT32_A(c);
			set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
			set->offsets[set->nattrs] = NEMOSHOW_ITEM_ALPHA_COLOR;
			set->dirties[set->nattrs] = prop->dirty;
			set->states[set->nattrs] = prop->state;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			return set->nattrs;
		}
	}

	return -1;
}

int nemoshow_sequence_set_dattr(struct showone *one, const char *name, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		sattr = nemoobject_get(&src->object, name);
		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = value;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = 0;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;

		return set->nattrs++;
	}

	return -1;
}

int nemoshow_sequence_set_dattr_offset(struct showone *one, const char *name, int offset, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		sattr = nemoobject_get(&src->object, name);
		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = value;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = offset;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;

		return set->nattrs++;
	}

	return -1;
}

int nemoshow_sequence_set_fattr(struct showone *one, const char *name, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		sattr = nemoobject_get(&src->object, name);
		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = value;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = 0;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_FLOAT_PROP;

		return set->nattrs++;
	}

	return -1;
}

int nemoshow_sequence_set_fattr_offset(struct showone *one, const char *name, int offset, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		sattr = nemoobject_get(&src->object, name);
		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = value;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = offset;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_FLOAT_PROP;

		return set->nattrs++;
	}

	return -1;
}

int nemoshow_sequence_set_cattr(struct showone *one, const char *name, double r, double g, double b, double a)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct nemoattr *sattr;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		sattr = nemoobject_get(&src->object, name);

		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = r;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = NEMOSHOW_ITEM_RED_COLOR;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
		set->nattrs++;

		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = g;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = NEMOSHOW_ITEM_GREEN_COLOR;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
		set->nattrs++;

		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = b;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = NEMOSHOW_ITEM_BLUE_COLOR;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
		set->nattrs++;

		set->attrs[set->nattrs] = sattr;
		set->eattrs[set->nattrs] = a;
		set->fattrs[set->nattrs] = set->eattrs[set->nattrs];
		set->offsets[set->nattrs] = NEMOSHOW_ITEM_ALPHA_COLOR;
		set->dirties[set->nattrs] = prop->dirty;
		set->states[set->nattrs] = prop->state;
		set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
		set->nattrs++;

		return set->nattrs;
	}

	return -1;
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

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t state = 0x0;

			set = NEMOSHOW_SET(child);

			for (i = 0; i < set->nattrs; i++) {
				if (nemoattr_get_serial(set->attrs[i]) <= serial) {
					if (set->types[i] == NEMOSHOW_DOUBLE_PROP)
						set->sattrs[i] = nemoattr_getd_offset(set->attrs[i], set->offsets[i]);
					else
						set->sattrs[i] = nemoattr_getf_offset(set->attrs[i], set->offsets[i]);

					state |= set->states[i];
				}
			}

			nemoshow_one_set_state(set->src, state);
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

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(child);

			for (i = 0; i < set->nattrs; i++) {
				if (nemoattr_get_serial(set->attrs[i]) <= serial) {
					if (set->types[i] == NEMOSHOW_DOUBLE_PROP)
						nemoattr_setd_offset(set->attrs[i], set->offsets[i],
								(set->eattrs[i] - set->sattrs[i]) * dt + set->sattrs[i]);
					else
						nemoattr_setf_offset(set->attrs[i], set->offsets[i],
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

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(child);

			for (i = 0; i < set->nattrs; i++) {
				if (nemoattr_get_serial(set->attrs[i]) <= serial) {
					if (set->types[i] == NEMOSHOW_DOUBLE_PROP)
						nemoattr_setd_offset(set->attrs[i], set->offsets[i], set->fattrs[i]);
					else
						nemoattr_setf_offset(set->attrs[i], set->offsets[i], set->fattrs[i]);

					dirty |= set->dirties[i];
				}
			}

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

	sequence->frame = nemoshow_children0(one);

	nemoshow_children_for_each(frame, one) {
		nemoshow_children_for_each(child, frame) {
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

			sequence->frame = nemoshow_children_next(one, sequence->frame);
			if (sequence->frame != NULL)
				nemoshow_sequence_prepare_frame(sequence->frame, serial);
		} else {
			nemoshow_sequence_dispatch_frame(sequence->frame, sequence->t, t, serial);
		}
	}
}
