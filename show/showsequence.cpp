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

	nemolist_init(&set->act_list);

	return one;
}

void nemoshow_sequence_destroy_set(struct showone *one)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showact *act, *nact;

	nemoshow_one_finish(one);

	nemolist_for_each_safe(act, nact, &set->act_list, link) {
		nemolist_remove(&act->link);

		free(act);
	}

	free(set);
}

int nemoshow_sequence_update_set(struct showone *one)
{
	return 0;
}

static inline int nemoshow_sequence_append_act(struct showone *one, struct nemoattr *attr, double e, double f, uint32_t offset, uint32_t dirty, uint32_t state, int type)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showact *act;

	act = (struct showact *)malloc(sizeof(struct showact));
	act->attr = attr;
	act->eattr = e;
	act->fattr = f;
	act->offset = offset;
	act->dirty = dirty;
	act->state = state;
	act->type = type;

	nemolist_insert(&set->act_list, &act->link);

	return set->act_count++;
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

			return nemoshow_sequence_append_act(one,
					nemoobject_get(&src->object, name),
					d, d,
					0,
					prop->dirty, prop->state,
					NEMOSHOW_DOUBLE_PROP);
		} else if (prop->type == NEMOSHOW_FLOAT_PROP) {
			double d = strtod(value, NULL);

			return nemoshow_sequence_append_act(one,
					nemoobject_get(&src->object, name),
					d, d,
					0,
					prop->dirty, prop->state,
					NEMOSHOW_FLOAT_PROP);
		} else if (prop->type == NEMOSHOW_COLOR_PROP) {
			uint32_t c = nemoshow_color_parse(value);

			sattr = nemoobject_get(&src->object, name);

			nemoshow_sequence_append_act(one,
					sattr,
					(double)NEMOSHOW_COLOR_UINT32_R(c),
					(double)NEMOSHOW_COLOR_UINT32_R(c),
					NEMOSHOW_RED_COLOR,
					prop->dirty, prop->state,
					NEMOSHOW_DOUBLE_PROP);

			nemoshow_sequence_append_act(one,
					sattr,
					(double)NEMOSHOW_COLOR_UINT32_G(c),
					(double)NEMOSHOW_COLOR_UINT32_G(c),
					NEMOSHOW_GREEN_COLOR,
					prop->dirty, prop->state,
					NEMOSHOW_DOUBLE_PROP);

			nemoshow_sequence_append_act(one,
					sattr,
					(double)NEMOSHOW_COLOR_UINT32_B(c),
					(double)NEMOSHOW_COLOR_UINT32_B(c),
					NEMOSHOW_BLUE_COLOR,
					prop->dirty, prop->state,
					NEMOSHOW_DOUBLE_PROP);

			nemoshow_sequence_append_act(one,
					sattr,
					(double)NEMOSHOW_COLOR_UINT32_A(c),
					(double)NEMOSHOW_COLOR_UINT32_A(c),
					NEMOSHOW_ALPHA_COLOR,
					prop->dirty, prop->state,
					NEMOSHOW_DOUBLE_PROP);

			return set->act_count;
		}
	}

	return -1;
}

int nemoshow_sequence_set_dattr(struct showone *one, const char *name, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		return nemoshow_sequence_append_act(one,
				nemoobject_get(&src->object, name),
				value,
				value,
				0,
				prop->dirty, prop->state,
				NEMOSHOW_DOUBLE_PROP);
	}

	return nemoshow_sequence_append_act(one,
			nemoobject_get(&src->object, name),
			value,
			value,
			0,
			0x0, 0x0,
			NEMOSHOW_DOUBLE_PROP);
}

int nemoshow_sequence_set_dattr_offset(struct showone *one, const char *name, int offset, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		return nemoshow_sequence_append_act(one,
				nemoobject_get(&src->object, name),
				value,
				value,
				offset,
				prop->dirty, prop->state,
				NEMOSHOW_DOUBLE_PROP);
	}

	return -1;
}

int nemoshow_sequence_set_fattr(struct showone *one, const char *name, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		return nemoshow_sequence_append_act(one,
				nemoobject_get(&src->object, name),
				value,
				value,
				0,
				prop->dirty, prop->state,
				NEMOSHOW_FLOAT_PROP);
	}

	return nemoshow_sequence_append_act(one,
			nemoobject_get(&src->object, name),
			value,
			value,
			0,
			0x0, 0x0,
			NEMOSHOW_FLOAT_PROP);
}

int nemoshow_sequence_set_fattr_offset(struct showone *one, const char *name, int offset, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct showprop *prop;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		return nemoshow_sequence_append_act(one,
				nemoobject_get(&src->object, name),
				value,
				value,
				offset,
				prop->dirty, prop->state,
				NEMOSHOW_FLOAT_PROP);
	}

	return -1;
}

int nemoshow_sequence_set_cattr(struct showone *one, const char *name, double r, double g, double b, double a)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showone *src = set->src;
	struct showprop *prop;
	struct nemoattr *sattr;

	prop = nemoshow_get_property(name);
	if (prop != NULL) {
		sattr = nemoobject_get(&src->object, name);

		nemoshow_sequence_append_act(one,
				sattr,
				r, r,
				NEMOSHOW_RED_COLOR,
				prop->dirty, prop->state,
				NEMOSHOW_DOUBLE_PROP);

		nemoshow_sequence_append_act(one,
				sattr,
				g, g,
				NEMOSHOW_GREEN_COLOR,
				prop->dirty, prop->state,
				NEMOSHOW_DOUBLE_PROP);

		nemoshow_sequence_append_act(one,
				sattr,
				b, b,
				NEMOSHOW_BLUE_COLOR,
				prop->dirty, prop->state,
				NEMOSHOW_DOUBLE_PROP);

		nemoshow_sequence_append_act(one,
				sattr,
				a, a,
				NEMOSHOW_ALPHA_COLOR,
				prop->dirty, prop->state,
				NEMOSHOW_DOUBLE_PROP);

		return set->act_count;
	}

	return -1;
}

int nemoshow_sequence_fix_dattr(struct showone *one, int index, double value)
{
	struct showset *set = NEMOSHOW_SET(one);
	struct showact *act;

	nemolist_for_each(act, &set->act_list, link) {
		if (index-- <= 0) {
			act->fattr = value;
			break;
		}
	}

	return 0;
}

static void nemoshow_sequence_prepare_frame(struct showone *one, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	struct showone *child;
	struct showact *act;

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t state = 0x0;

			set = NEMOSHOW_SET(child);

			nemolist_for_each(act, &set->act_list, link) {
				if (nemoattr_get_serial(act->attr) <= serial) {
					if (act->type == NEMOSHOW_DOUBLE_PROP)
						act->sattr = nemoattr_getd_offset(act->attr, act->offset);
					else
						act->sattr = nemoattr_getf_offset(act->attr, act->offset);

					state |= act->state;
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
	struct showact *act;
	double dt = (t - s) / (frame->t - s);

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(child);

			nemolist_for_each(act, &set->act_list, link) {
				if (nemoattr_get_serial(act->attr) <= serial) {
					double v = (act->eattr - act->sattr) * dt + act->sattr;

					if (act->type == NEMOSHOW_DOUBLE_PROP)
						nemoattr_setd_offset(act->attr, act->offset, v);
					else
						nemoattr_setf_offset(act->attr, act->offset, v);

					if (set->src->dattr != NULL)
						set->src->dattr(set->src, nemoattr_get_name(act->attr), v);

					dirty |= act->dirty;
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
	struct showact *act;

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(child);

			nemolist_for_each(act, &set->act_list, link) {
				if (nemoattr_get_serial(act->attr) <= serial) {
					double v = act->fattr;

					if (act->type == NEMOSHOW_DOUBLE_PROP)
						nemoattr_setd_offset(act->attr, act->offset, v);
					else
						nemoattr_setf_offset(act->attr, act->offset, v);

					if (set->src->dattr != NULL)
						set->src->dattr(set->src, nemoattr_get_name(act->attr), v);

					dirty |= act->dirty;
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
	struct showact *act;

	if (one->serial > serial)
		return;

	one->serial = serial;

	sequence->t = 0.0f;
	sequence->frame = NULL;

	nemoshow_children_for_each(frame, one) {
		nemoshow_children_for_each(child, frame) {
			if (child->type == NEMOSHOW_SET_TYPE) {
				struct showset *set = NEMOSHOW_SET(child);

				nemolist_for_each(act, &set->act_list, link) {
					if (nemoattr_get_serial(act->attr) < serial)
						nemoattr_set_serial(act->attr, serial);
				}
			}
		}
	}
}

void nemoshow_sequence_dispatch(struct showone *one, double t, uint32_t serial)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);

	if (one->serial > serial)
		return;

	if (sequence->frame == NULL) {
		struct showone *frame;

		nemoshow_children_for_each(frame, one) {
			sequence->frame = frame;

			if (NEMOSHOW_FRAME_AT(frame, t) > t)
				break;
		}

		nemoshow_sequence_prepare_frame(sequence->frame, serial);
	}

	if (sequence->frame != NULL) {
		struct showframe *frame = NEMOSHOW_FRAME(sequence->frame);

		if (t >= 1.0f) {
			nemoshow_sequence_finish_frame(sequence->frame, serial);
		} else if (frame->t < t) {
			nemoshow_sequence_finish_frame(sequence->frame, serial);

			sequence->t = frame->t;
			sequence->frame = NULL;
		} else {
			nemoshow_sequence_dispatch_frame(sequence->frame, sequence->t, t, serial);
		}
	}
}
