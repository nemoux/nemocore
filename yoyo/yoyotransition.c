#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyotransition.h>
#include <nemoyoyo.h>
#include <yoyoone.h>
#include <nemomisc.h>

#define NEMOYOYO_TRANSITION_ONE_TARGETS_MAX			(8)

struct transitionone {
	float *toattr;
	uint32_t *todirty;

	uint32_t dirty;

	float timings[NEMOYOYO_TRANSITION_ONE_TARGETS_MAX];
	float targets[NEMOYOYO_TRANSITION_ONE_TARGETS_MAX];
	int count;
};

struct transitionsensor {
	struct nemolist link;
	struct nemolistener listener;

	struct yoyotransition *transition;
	struct yoyoone *one;
};

struct yoyotransition *nemoyoyo_transition_create(int max, int type, uint32_t duration, uint32_t delay)
{
	struct yoyotransition *trans;

	trans = (struct yoyotransition *)malloc(sizeof(struct yoyotransition));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct yoyotransition));

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

void nemoyoyo_transition_destroy(struct yoyotransition *trans)
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

	free(trans);
}

void nemoyoyo_transition_ease_set_type(struct yoyotransition *trans, int type)
{
	nemoease_set(&trans->ease, type);
}

void nemoyoyo_transition_ease_set_bezier(struct yoyotransition *trans, double x0, double y0, double x1, double y1)
{
	nemoease_set_cubic(&trans->ease, x0, y0, x1, y1);
}

void nemoyoyo_transition_set_repeat(struct yoyotransition *trans, uint32_t repeat)
{
	trans->repeat = repeat;
}

int nemoyoyo_transition_check_repeat(struct yoyotransition *trans)
{
	if (trans->repeat == 0 || --trans->repeat > 0) {
		trans->stime = 0;
		trans->etime = 0;

		return 0;
	}

	return 1;
}

static void nemoyoyo_transition_handle_one_destroy(struct nemolistener *listener, void *data)
{
	struct transitionsensor *sensor = (struct transitionsensor *)container_of(listener, struct transitionsensor, listener);
	struct yoyotransition *trans = sensor->transition;
	struct yoyoone *one = sensor->one;

	nemoyoyo_transition_put_attr(trans, one, one->size);

	nemolist_remove(&sensor->link);
	nemolist_remove(&sensor->listener.link);

	free(sensor);
}

void nemoyoyo_transition_check_one(struct yoyotransition *trans, struct yoyoone *one)
{
	struct transitionsensor *sensor;

	sensor = (struct transitionsensor *)malloc(sizeof(struct transitionsensor));
	sensor->transition = trans;
	sensor->one = one;

	sensor->listener.notify = nemoyoyo_transition_handle_one_destroy;
	nemosignal_add(&one->destroy_signal, &sensor->listener);

	nemolist_insert_tail(&trans->sensor_list, &sensor->link);
}

int nemoyoyo_transition_set_attr(struct yoyotransition *trans, int index, float *toattr, float attr, uint32_t *todirty, uint32_t dirty)
{
	struct transitionone *one;

	trans->ones[index] = one = (struct transitionone *)malloc(sizeof(struct transitionone));
	if (one == NULL)
		return -1;
	memset(one, 0, sizeof(struct transitionone));

	one->count = 0;
	one->toattr = toattr;
	one->todirty = todirty;
	one->dirty = dirty;

	nemoyoyo_transition_set_target(trans, index, 0.0f, attr);

	trans->nones = MAX(trans->nones, index + 1);

	return 0;
}

void nemoyoyo_transition_put_attr(struct yoyotransition *trans, void *var, int size)
{
	struct transitionone *one;
	int i;

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL && var <= (void *)one->toattr && (void *)one->toattr < var + size) {
			trans->ones[i] = NULL;
			free(one);
		}
	}
}

void nemoyoyo_transition_set_target(struct yoyotransition *trans, int index, float t, float v)
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

int nemoyoyo_transition_dispatch(struct yoyotransition *trans, uint32_t msecs)
{
	struct transitionone *one;
	float t, u, v;
	int i, j;

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

			*one->toattr = v;
			*one->todirty |= one->dirty;
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
