#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotransition.h>
#include <nemomisc.h>

#define NEMOTRANSITION_ONE_TARGETS_MAX			(8)

struct transitionone {
	int is_double;
	void *var;

	union {
		float f;
		double d;
	} v;

	uint32_t *obj;
	uint32_t dirty;

	double timings[NEMOTRANSITION_ONE_TARGETS_MAX];
	double targets[NEMOTRANSITION_ONE_TARGETS_MAX];
	int count;
};

struct transitionsensor {
	struct nemolist link;
	struct nemolistener listener;

	struct nemotransition *transition;
	void *var;
	int size;
};

struct transitiongroup *nemotransition_group_create(void)
{
	struct transitiongroup *group;

	group = (struct transitiongroup *)malloc(sizeof(struct transitiongroup));
	if (group == NULL)
		return NULL;
	memset(group, 0, sizeof(struct transitiongroup));

	nemolist_init(&group->list);

	return group;
}

void nemotransition_group_destroy(struct transitiongroup *group)
{
	free(group);
}

void nemotransition_group_attach_transition(struct transitiongroup *group, struct nemotransition *trans)
{
	struct nemotransition *strans;
	struct transitionone *one, *sone;
	int i, j;

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL) {
			nemolist_for_each(strans, &group->list, link) {
				for (j = 0; j < strans->nones; j++) {
					sone = strans->ones[j];
					if (sone != NULL) {
						if (one->var == sone->var) {
							strans->ones[j] = NULL;
							free(sone);
							break;
						}
					}
				}
			}
		}
	}

	nemolist_insert_tail(&group->list, &trans->link);
}

void nemotransition_group_detach_transition(struct transitiongroup *group, struct nemotransition *trans)
{
	nemolist_remove(&trans->link);
}

void nemotransition_group_ready(struct transitiongroup *group, uint32_t msecs)
{
	struct nemotransition *trans;

	nemolist_for_each(trans, &group->list, link) {
		nemotransition_ready(trans, msecs);
	}
}

int nemotransition_group_dispatch(struct transitiongroup *group, uint32_t msecs)
{
	struct nemotransition *trans, *ntrans;
	int has_transition = nemolist_empty(&group->list) == 0;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		if (nemotransition_dispatch(trans, msecs) != 0) {
			if (nemotransition_put_repeat(trans) > 0)
				nemotransition_destroy(trans);
		}
	}

	return has_transition;
}

struct nemotransition *nemotransition_group_get_last_one(struct transitiongroup *group, void *var)
{
	struct nemotransition *ltrans = NULL;
	struct nemotransition *trans;
	struct transitionone *one;
	int i;

	nemolist_for_each(trans, &group->list, link) {
		for (i = 0; i < trans->nones; i++) {
			one = trans->ones[i];
			if (one != NULL) {
				if (one->var == var) {
					if (ltrans == NULL || ltrans->etime < trans->etime) {
						ltrans = trans;
						break;
					}
				}
			}
		}
	}

	return ltrans;
}

struct nemotransition *nemotransition_group_get_last_tag(struct transitiongroup *group, uint32_t tag)
{
	struct nemotransition *ltrans = NULL;
	struct nemotransition *trans;

	nemolist_for_each(trans, &group->list, link) {
		if (trans->tag == tag) {
			if (ltrans == NULL || ltrans->etime < trans->etime)
				ltrans = trans;
		}
	}

	return ltrans;
}

struct nemotransition *nemotransition_group_get_last_all(struct transitiongroup *group)
{
	struct nemotransition *ltrans = NULL;
	struct nemotransition *trans;

	nemolist_for_each(trans, &group->list, link) {
		if (ltrans == NULL || ltrans->etime < trans->etime)
			ltrans = trans;
	}

	return ltrans;
}

void nemotransition_group_revoke_one(struct transitiongroup *group, void *var, int size)
{
	struct nemotransition *trans;
	struct transitionone *one;
	int i;

	nemolist_for_each(trans, &group->list, link) {
		for (i = 0; i < trans->nones; i++) {
			one = trans->ones[i];
			if (one != NULL && var <= one->var && one->var < var + size) {
				trans->ones[i] = NULL;
				free(one);
			}
		}
	}
}

void nemotransition_group_revoke_tag(struct transitiongroup *group, uint32_t tag)
{
	struct nemotransition *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		if (trans->tag == tag)
			nemotransition_destroy(trans);
	}
}

void nemotransition_group_revoke_all(struct transitiongroup *group)
{
	struct nemotransition *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		nemotransition_destroy(trans);
	}
}

struct nemotransition *nemotransition_create(int max, int type, uint32_t duration, uint32_t delay)
{
	struct nemotransition *trans;

	trans = (struct nemotransition *)malloc(sizeof(struct nemotransition));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct nemotransition));

	trans->ones = (struct transitionone **)malloc(sizeof(struct transitionone *) * max);
	if (trans->ones == NULL)
		goto err1;
	memset(trans->ones, 0, sizeof(struct transitionone *) * max);

	trans->sones = max;
	trans->nones = 0;

	nemolist_init(&trans->link);

	nemolist_init(&trans->sensor_list);

	nemoease_set(&trans->ease, type);

	trans->duration = duration;
	trans->delay = delay;

	trans->repeat = 1;

	trans->stime = 0;
	trans->etime = 0;

	return trans;

err1:
	free(trans);

	return NULL;
}

void nemotransition_destroy(struct nemotransition *trans)
{
	struct transitionsensor *sensor, *nsensor;
	int i;

	nemolist_remove(&trans->link);

	nemolist_for_each_safe(sensor, nsensor, &trans->sensor_list, link) {
		nemolist_remove(&sensor->link);
		nemolist_remove(&sensor->listener.link);

		free(sensor);
	}

	for (i = 0; i < trans->nones; i++) {
		if (trans->ones[i] != NULL)
			free(trans->ones[i]);
	}

	free(trans->ones);
	free(trans);
}

void nemotransition_set_tag(struct nemotransition *trans, uint32_t tag)
{
	trans->tag = tag;
}

void nemotransition_ease_set_type(struct nemotransition *trans, int type)
{
	nemoease_set(&trans->ease, type);
}

void nemotransition_ease_set_bezier(struct nemotransition *trans, double x0, double y0, double x1, double y1)
{
	nemoease_set_cubic(&trans->ease, x0, y0, x1, y1);
}

void nemotransition_set_repeat(struct nemotransition *trans, uint32_t repeat)
{
	trans->repeat = repeat;
}

int nemotransition_put_repeat(struct nemotransition *trans)
{
	if (trans->done != 0)
		return 1;

	if (trans->repeat == 0 || --trans->repeat > 0) {
		trans->stime = 0;
		trans->etime = 0;

		return 0;
	}

	return 1;
}

int nemotransition_ready(struct nemotransition *trans, uint32_t msecs)
{
	if (trans->stime == 0) {
		trans->stime = msecs + trans->delay;
		trans->etime = msecs + trans->delay + trans->duration;
	}

	return 0;
}

int nemotransition_dispatch(struct nemotransition *trans, uint32_t msecs)
{
	struct transitionone *one;
	double t, u, v;
	int i, j;

	if (trans->done != 0)
		return 1;

	if (trans->stime == 0) {
		trans->stime = msecs + trans->delay;
		trans->etime = msecs + trans->delay + trans->duration;
	}

	if (trans->stime > msecs)
		return 0;

	t = nemoease_get(&trans->ease, msecs - trans->stime, trans->duration);

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL) {
			for (j = 0; j < one->count - 1; j++) {
				if (one->timings[j] <= t && t < one->timings[j + 1]) {
					u = (t - one->timings[j]) / (one->timings[j + 1] - one->timings[j]);
					v = (one->targets[j + 1] - one->targets[j]) * u + one->targets[j];
					break;
				}
			}

			if (j >= one->count - 1)
				v = one->targets[one->count - 1] * (t / one->timings[one->count - 1]);

			if (one->is_double == 0)
				*((float *)one->var) = v;
			else
				*((double *)one->var) = v;

			if (one->obj != NULL)
				*one->obj |= one->dirty;
		}
	}

	if (trans->dispatch_update != NULL)
		trans->dispatch_update(trans, trans->data, t);

	if (trans->etime <= msecs) {
		if (trans->dispatch_done != NULL)
			trans->dispatch_done(trans, trans->data);

		return 1;
	}

	return 0;
}

void nemotransition_set_float(struct nemotransition *trans, int index, float *var, float attr)
{
	struct transitionone *one;

	trans->ones[index] = one = (struct transitionone *)malloc(sizeof(struct transitionone));
	one->count = 0;
	one->is_double = 0;
	if (var != NULL)
		one->var = var;
	else
		one->var = &one->v.f;
	one->obj = NULL;

	nemotransition_set_target(trans, index, 0.0f, attr);

	trans->nones = MAX(trans->nones, index + 1);
}

void nemotransition_set_double(struct nemotransition *trans, int index, double *var, double attr)
{
	struct transitionone *one;

	trans->ones[index] = one = (struct transitionone *)malloc(sizeof(struct transitionone));
	one->count = 0;
	one->is_double = 1;
	if (var != NULL)
		one->var = var;
	else
		one->var = &one->v.d;
	one->obj = NULL;

	nemotransition_set_target(trans, index, 0.0f, attr);

	trans->nones = MAX(trans->nones, index + 1);
}

void nemotransition_set_float_with_dirty(struct nemotransition *trans, int index, float *var, float attr, uint32_t *obj, uint32_t dirty)
{
	struct transitionone *one;

	trans->ones[index] = one = (struct transitionone *)malloc(sizeof(struct transitionone));
	one->count = 0;
	one->is_double = 0;
	if (var != NULL)
		one->var = var;
	else
		one->var = &one->v.f;
	one->obj = obj;
	one->dirty = dirty;

	nemotransition_set_target(trans, index, 0.0f, attr);

	trans->nones = MAX(trans->nones, index + 1);
}

void nemotransition_set_double_with_dirty(struct nemotransition *trans, int index, double *var, double attr, uint32_t *obj, uint32_t dirty)
{
	struct transitionone *one;

	trans->ones[index] = one = (struct transitionone *)malloc(sizeof(struct transitionone));
	one->count = 0;
	one->is_double = 1;
	if (var != NULL)
		one->var = var;
	else
		one->var = &one->v.d;
	one->obj = obj;
	one->dirty = dirty;

	nemotransition_set_target(trans, index, 0.0f, attr);

	trans->nones = MAX(trans->nones, index + 1);
}

float nemotransition_get_float(struct nemotransition *trans, int index)
{
	return *((float *)trans->ones[index]->var);
}

double nemotransition_get_double(struct nemotransition *trans, int index)
{
	return *((double *)trans->ones[index]->var);
}

void nemotransition_set_target(struct nemotransition *trans, int index, double t, double v)
{
	struct transitionone *one = trans->ones[index];
	int i, j;

	for (i = 0; i < one->count; i++) {
		if (t == one->timings[i]) {
			one->targets[i] = v;
			break;
		} else if (t < one->timings[i]) {
			for (j = i; j < one->count; j++) {
				one->timings[j + 1] = one->timings[j];
				one->targets[j + 1] = one->targets[j];
			}

			one->timings[i] = t;
			one->targets[i] = v;
			one->count++;
			break;
		}
	}

	if (i >= one->count) {
		one->timings[one->count] = t;
		one->targets[one->count] = v;
		one->count++;
	}
}

void nemotransition_put_one(struct nemotransition *trans, void *var, int size)
{
	struct transitionone *one;
	int i;

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL && var <= one->var && one->var < var + size) {
			trans->ones[i] = NULL;
			free(one);
		}
	}
}

static void nemotransition_handle_object_destroy(struct nemolistener *listener, void *data)
{
	struct transitionsensor *sensor = (struct transitionsensor *)container_of(listener, struct transitionsensor, listener);

	nemotransition_terminate(sensor->transition);

	nemolist_remove(&sensor->link);
	nemolist_remove(&sensor->listener.link);

	free(sensor);
}

void nemotransition_check_object_destroy(struct nemotransition *trans, struct nemosignal *signal)
{
	struct transitionsensor *sensor;

	sensor = (struct transitionsensor *)malloc(sizeof(struct transitionsensor));
	sensor->transition = trans;

	sensor->listener.notify = nemotransition_handle_object_destroy;
	nemosignal_add(signal, &sensor->listener);

	nemolist_insert_tail(&trans->sensor_list, &sensor->link);
}

static void nemotransition_handle_object_revoke(struct nemolistener *listener, void *data)
{
	struct transitionsensor *sensor = (struct transitionsensor *)container_of(listener, struct transitionsensor, listener);

	nemotransition_put_one(sensor->transition, sensor->var, sensor->size);

	nemolist_remove(&sensor->link);
	nemolist_remove(&sensor->listener.link);

	free(sensor);
}

void nemotransition_check_object_revoke(struct nemotransition *trans, struct nemosignal *signal, void *var, int size)
{
	struct transitionsensor *sensor;

	sensor = (struct transitionsensor *)malloc(sizeof(struct transitionsensor));
	sensor->transition = trans;
	sensor->var = var;
	sensor->size = size;

	sensor->listener.notify = nemotransition_handle_object_revoke;
	nemosignal_add(signal, &sensor->listener);

	nemolist_insert_tail(&trans->sensor_list, &sensor->link);
}
