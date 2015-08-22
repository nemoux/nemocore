#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
				set->tattrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_iget(&one->object, i);
				set->dirties[set->nattrs] = prop->dirty;
				set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
				set->nattrs++;
			} else if (prop->type == NEMOSHOW_COLOR_PROP) {
				char atname[NEMOSHOW_ATTR_NAME_MAX];

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->tattrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->tattrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->tattrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
				set->nattrs++;

				snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
				attr = nemoobject_get(&src->object, atname);
				if (attr == NULL)
					continue;
				set->tattrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
				set->dirties[set->nattrs] = prop->dirty;
				set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
				set->nattrs++;
			} else if (prop->type == NEMOSHOW_STRING_PROP) {
				attr = nemoobject_get(&src->object, name);
				if (attr == NULL)
					continue;
				set->tattrs[set->nattrs] = attr;
				set->eattrs[set->nattrs] = nemoobject_iget(&one->object, i);
				set->dirties[set->nattrs] = prop->dirty;
				set->types[set->nattrs] = NEMOSHOW_STRING_PROP;
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
		if (prop->type == NEMOSHOW_STRING_PROP) {
			nemoobject_sets(&one->object, name, value, strlen(value));
		} else if (prop->type == NEMOSHOW_DOUBLE_PROP) {
			nemoobject_setd(&one->object, name, strtod(value, NULL));
		} else if (prop->type == NEMOSHOW_INTEGER_PROP) {
			nemoobject_seti(&one->object, name, strtoul(value, NULL, 10));
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
			set->tattrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_get(&one->object, name);
			set->dirties[set->nattrs] = prop->dirty;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;
		} else if (prop->type == NEMOSHOW_COLOR_PROP) {
			char atname[NEMOSHOW_ATTR_NAME_MAX];

			snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:r", name);
			sattr = nemoobject_get(&src->object, atname);
			set->tattrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
			set->dirties[set->nattrs] = prop->dirty;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:g", name);
			sattr = nemoobject_get(&src->object, atname);
			set->tattrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
			set->dirties[set->nattrs] = prop->dirty;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:b", name);
			sattr = nemoobject_get(&src->object, atname);
			set->tattrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
			set->dirties[set->nattrs] = prop->dirty;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;

			snprintf(atname, NEMOSHOW_ATTR_NAME_MAX, "%s:a", name);
			sattr = nemoobject_get(&src->object, atname);
			set->tattrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_get(&one->object, atname);
			set->dirties[set->nattrs] = prop->dirty;
			set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
			set->nattrs++;
		} else if (prop->type == NEMOSHOW_STRING_PROP) {
			sattr = nemoobject_get(&src->object, name);
			set->tattrs[set->nattrs] = sattr;
			set->eattrs[set->nattrs] = nemoobject_get(&one->object, name);
			set->dirties[set->nattrs] = prop->dirty;
			set->types[set->nattrs] = NEMOSHOW_STRING_PROP;
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
	set->tattrs[set->nattrs] = sattr;
	set->eattrs[set->nattrs] = nemoobject_get(&one->object, name);
	set->dirties[set->nattrs] = dirty;
	set->types[set->nattrs] = NEMOSHOW_DOUBLE_PROP;
	set->nattrs++;

	return 0;
}

struct showone *nemoshow_sequence_create_follow(void)
{
	struct showfollow *follow;
	struct showone *one;

	follow = (struct showfollow *)malloc(sizeof(struct showfollow));
	if (follow == NULL)
		return NULL;
	memset(follow, 0, sizeof(struct showfollow));

	one = &follow->base;
	one->type = NEMOSHOW_FOLLOW_TYPE;
	one->update = nemoshow_sequence_update_follow;
	one->destroy = nemoshow_sequence_destroy_follow;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "from", &follow->from, sizeof(double));
	nemoobject_set_reserved(&one->object, "to", &follow->to, sizeof(double));

	return one;
}

void nemoshow_sequence_destroy_follow(struct showone *one)
{
	struct showfollow *follow = NEMOSHOW_FOLLOW(one);

	nemoshow_one_finish(one);

	free(follow);
}

int nemoshow_sequence_arrange_follow(struct nemoshow *show, struct showone *one)
{
	struct showfollow *follow = NEMOSHOW_FOLLOW(one);
	struct showone *src;
	struct showone *path;
	struct nemoattr *attr;
	const char *element;

	src = nemoshow_search_one(show, nemoobject_gets(&one->object, "src"));
	if (src == NULL)
		return -1;

	attr = nemoobject_get(&src->object, nemoobject_gets(&one->object, "attr"));
	if (attr == NULL)
		return -1;

	path = nemoshow_search_one(show, nemoobject_gets(&one->object, "path"));
	if (path == NULL)
		return -1;

	element = nemoobject_gets(&one->object, "element");
	if (element == NULL)
		return -1;

	if (strcmp(element, "x") == 0) {
		follow->element = NEMOSHOW_PATH_X_FOLLOW;
	} else if (strcmp(element, "y") == 0) {
		follow->element = NEMOSHOW_PATH_Y_FOLLOW;
	} else if (strcmp(element, "r") == 0) {
		follow->element = NEMOSHOW_PATH_R_FOLLOW;
	}

	follow->src = src;
	follow->attr = attr;
	follow->path = path;

	return 0;
}

int nemoshow_sequence_update_follow(struct nemoshow *show, struct showone *one)
{
	return 0;
}

static void nemoshow_sequence_prepare_frame(struct showone *one, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	struct showfollow *follow;
	int i, j;

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_SET_TYPE) {
			set = NEMOSHOW_SET(one->children[i]);

			for (j = 0; j < set->nattrs; j++) {
				if (nemoattr_get_serial(set->tattrs[j]) <= serial) {
					if (set->types[j] == NEMOSHOW_DOUBLE_PROP) {
						set->sattrs[j] = nemoattr_getd(set->tattrs[j]);
					}
				}
			}

			set->src->state = NEMOSHOW_TRANSITION_STATE;
		} else if (one->children[i]->type == NEMOSHOW_FOLLOW_TYPE) {
			follow = NEMOSHOW_FOLLOW(one->children[i]);

			follow->src->state = NEMOSHOW_TRANSITION_STATE;
		}
	}
}

static void nemoshow_sequence_dispatch_frame(struct showone *one, double s, double t, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	struct showfollow *follow;
	double dt = (t - s) / (frame->t - s);
	int i, j;

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(one->children[i]);

			for (j = 0; j < set->nattrs; j++) {
				if (nemoattr_get_serial(set->tattrs[j]) <= serial) {
					if (set->types[j] == NEMOSHOW_DOUBLE_PROP) {
						double d = (nemoattr_getd(set->eattrs[j]) - set->sattrs[j]) * dt + set->sattrs[j];

						nemoattr_setd(set->tattrs[j], d);
					}

					dirty |= set->dirties[j];
				}
			}

			nemoshow_one_dirty(set->src, dirty);
		} else if (one->children[i]->type == NEMOSHOW_FOLLOW_TYPE) {
			follow = NEMOSHOW_FOLLOW(one->children[i]);

			if (nemoattr_get_serial(follow->attr) <= serial) {
				double x, y, r;

				nemoshow_helper_evaluate_path(NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(follow->path), path),
						NEMOSHOW_ITEM_AT(follow->path, length),
						(follow->to - follow->from) * t + follow->from,
						&x, &y, &r);

				if (follow->element == NEMOSHOW_PATH_X_FOLLOW) {
					nemoattr_setd(follow->attr, x);
				} else if (follow->element == NEMOSHOW_PATH_Y_FOLLOW) {
					nemoattr_setd(follow->attr, y);
				} else if (follow->element == NEMOSHOW_PATH_R_FOLLOW) {
					nemoattr_setd(follow->attr, r);
				}

				nemoshow_one_dirty(follow->src, NEMOSHOW_SHAPE_DIRTY);
			}
		}
	}
}

static void nemoshow_sequence_finish_frame(struct showone *one, uint32_t serial)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	struct showfollow *follow;
	int i, j;

	for (i = 0; i < one->nchildren; i++) {
		if (one->children[i]->type == NEMOSHOW_SET_TYPE) {
			uint32_t dirty = 0x0;

			set = NEMOSHOW_SET(one->children[i]);

			for (j = 0; j < set->nattrs; j++) {
				if (nemoattr_get_serial(set->tattrs[j]) <= serial) {
					if (set->types[j] == NEMOSHOW_DOUBLE_PROP) {
						double d = nemoattr_getd(set->eattrs[j]);

						nemoattr_setd(set->tattrs[j], d);
					} else if (set->types[j] == NEMOSHOW_STRING_PROP) {
						const char *s = nemoattr_gets(set->eattrs[j]);

						nemoattr_sets(set->tattrs[j], s, strlen(s));
					}

					dirty |= set->dirties[j];
				}
			}

			set->src->state = NEMOSHOW_NORMAL_STATE;

			nemoshow_one_dirty(set->src, dirty);
		} else if (one->children[i]->type == NEMOSHOW_FOLLOW_TYPE) {
			follow = NEMOSHOW_FOLLOW(one->children[i]);

			if (nemoattr_get_serial(follow->attr) <= serial) {
				double x, y, r;

				nemoshow_helper_evaluate_path(NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(follow->path), path),
						NEMOSHOW_ITEM_AT(follow->path, length),
						follow->to,
						&x, &y, &r);

				if (follow->element == NEMOSHOW_PATH_X_FOLLOW) {
					nemoattr_setd(follow->attr, x);
				} else if (follow->element == NEMOSHOW_PATH_Y_FOLLOW) {
					nemoattr_setd(follow->attr, y);
				} else if (follow->element == NEMOSHOW_PATH_R_FOLLOW) {
					nemoattr_setd(follow->attr, r);
				}

				nemoshow_one_dirty(follow->src, NEMOSHOW_SHAPE_DIRTY);
			}

			follow->src->state = NEMOSHOW_NORMAL_STATE;
		}
	}
}

void nemoshow_sequence_prepare(struct showone *one, uint32_t serial)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);
	struct showone *frame;
	int i, j, k;

	sequence->t = 0.0f;
	sequence->iframe = 0;

	one->serial = serial;

	for (i = 0; i < one->nchildren; i++) {
		frame = one->children[i];

		for (j = 0; j < frame->nchildren; j++) {
			if (frame->children[j]->type == NEMOSHOW_SET_TYPE) {
				struct showset *set = NEMOSHOW_SET(frame->children[j]);

				for (k = 0; k < set->nattrs; k++) {
					nemoattr_set_serial(set->tattrs[k], serial);
				}
			} else if (frame->children[j]->type == NEMOSHOW_FOLLOW_TYPE) {
				struct showfollow *follow = NEMOSHOW_FOLLOW(frame->children[j]);

				nemoattr_set_serial(follow->attr, serial);
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
