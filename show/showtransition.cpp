#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showtransition.h>
#include <showsequence.h>
#include <showmisc.h>
#include <nemomisc.h>

struct showtransition *nemoshow_transition_create(struct showone *ease, uint32_t duration, uint32_t delay)
{
	struct showtransition *trans;

	trans = (struct showtransition *)malloc(sizeof(struct showtransition));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct showtransition));

	nemolist_init(&trans->link);

	nemolist_init(&trans->sensor_list);
	nemolist_init(&trans->pin_list);

	trans->sequences = (struct showone **)malloc(sizeof(struct showone *) * 4);
	trans->nsequences = 0;
	trans->ssequences = 4;

	trans->dones = (struct showone **)malloc(sizeof(struct showone *) * 4);
	trans->ndones = 0;
	trans->sdones = 4;

	trans->dirties = (uint32_t *)malloc(sizeof(uint32_t) * 4);
	trans->ndirties = 0;
	trans->sdirties = 4;

	trans->tones = (struct showone **)malloc(sizeof(struct showone *) * 4);
	trans->ntones = 0;
	trans->stones = 4;

	trans->ease = NEMOSHOW_EASE(ease);

	trans->duration = duration;
	trans->delay = delay;
	trans->repeat = 1;

	return trans;
}

void nemoshow_transition_destroy(struct showtransition *trans)
{
	struct transitionsensor *sensor, *nsensor;
	struct transitionpin *pin, *npin;
	int i;

	nemolist_remove(&trans->link);

	nemolist_for_each_safe(sensor, nsensor, &trans->sensor_list, link) {
		nemolist_remove(&sensor->link);
		nemolist_remove(&sensor->listener.link);

		free(sensor);
	}

	nemolist_for_each_safe(pin, npin, &trans->pin_list, link) {
		nemolist_remove(&pin->link);
		nemolist_remove(&pin->listener.link);

		free(pin);
	}

	for (i = 0; i < trans->nsequences; i++) {
		nemoshow_one_destroy(trans->sequences[i]);
	}

	for (i = 0; i < trans->ntones; i++) {
		if (trans->tones[i] != NULL)
			nemoshow_one_destroy(trans->tones[i]);
	}

	free(trans->sequences);
	free(trans->dones);
	free(trans->dirties);
	free(trans->tones);
	free(trans);
}

static void nemoshow_transition_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct transitionsensor *sensor = (struct transitionsensor *)container_of(listener, struct transitionsensor, listener);
	struct showtransition *trans = sensor->transition;
	struct showone *one = sensor->one;
	int i;

	for (i = 0; i < trans->ntones; i++) {
		if (trans->tones[i] == one)
			trans->tones[i] = NULL;
	}

	trans->done = 1;

	nemolist_remove(&sensor->link);
	nemolist_remove(&sensor->listener.link);

	free(sensor);
}

void nemoshow_transition_check_one(struct showtransition *trans, struct showone *one)
{
	struct transitionsensor *sensor;

	nemolist_for_each(sensor, &trans->sensor_list, link) {
		if (sensor->one == one)
			return;
	}

	sensor = (struct transitionsensor *)malloc(sizeof(struct transitionsensor));
	if (sensor != NULL) {
		sensor->transition = trans;
		sensor->one = one;

		sensor->listener.notify = nemoshow_transition_handle_destroy_signal;
		nemosignal_add(&one->destroy_signal, &sensor->listener);

		nemolist_insert_tail(&trans->sensor_list, &sensor->link);
	}
}

static void nemoshow_transition_handle_dirty_unpin_signal(struct nemolistener *listener, void *data)
{
	struct transitionpin *pin = (struct transitionpin *)container_of(listener, struct transitionpin, listener);

	pin->transition->dones[pin->index] = NULL;

	nemolist_remove(&pin->link);
	nemolist_remove(&pin->listener.link);

	free(pin);
}

void nemoshow_transition_dirty_one(struct showtransition *trans, struct showone *one, uint32_t dirty)
{
	struct transitionpin *pin;

	pin = (struct transitionpin *)malloc(sizeof(struct transitionpin));
	if (pin != NULL) {
		NEMOSHOW_ARRAY_APPEND(trans->dones, trans->sdones, trans->ndones, one);
		NEMOSHOW_ARRAY_APPEND(trans->dirties, trans->sdirties, trans->ndirties, dirty);

		pin->transition = trans;
		pin->index = trans->ndones - 1;

		pin->listener.notify = nemoshow_transition_handle_dirty_unpin_signal;
		nemosignal_add(&one->unpin_signal, &pin->listener);

		nemolist_insert_tail(&trans->pin_list, &pin->link);
	}
}

static void nemoshow_transition_handle_destroy_unpin_signal(struct nemolistener *listener, void *data)
{
	struct transitionpin *pin = (struct transitionpin *)container_of(listener, struct transitionpin, listener);

	pin->transition->tones[pin->index] = NULL;

	nemolist_remove(&pin->link);
	nemolist_remove(&pin->listener.link);

	free(pin);
}

void nemoshow_transition_destroy_one(struct showtransition *trans, struct showone *one)
{
	struct transitionpin *pin;

	pin = (struct transitionpin *)malloc(sizeof(struct transitionpin));
	if (pin != NULL) {
		NEMOSHOW_ARRAY_APPEND(trans->tones, trans->stones, trans->ntones, one);

		pin->transition = trans;
		pin->index = trans->ntones - 1;

		pin->listener.notify = nemoshow_transition_handle_destroy_unpin_signal;
		nemosignal_add(&one->unpin_signal, &pin->listener);

		nemolist_insert_tail(&trans->pin_list, &pin->link);
	}
}

void nemoshow_transition_attach_sequence(struct showtransition *trans, struct showone *sequence)
{
	NEMOSHOW_ARRAY_APPEND(trans->sequences, trans->ssequences, trans->nsequences, sequence);
}

int nemoshow_transition_dispatch(struct showtransition *trans, uint32_t time)
{
	double t;
	int done = 0;
	int i;

	if (trans->done != 0)
		return 1;

	if (trans->stime == 0) {
		trans->stime = time + trans->delay;
		trans->etime = time + trans->delay + trans->duration;
	}

	if (trans->stime > time)
		return 0;

	if (trans->etime <= time) {
		t = 1.0f;
		done = 1;
	} else {
		t = nemoease_get(&trans->ease->ease, time - trans->stime, trans->duration);
	}

	for (i = 0; i < trans->nsequences; i++) {
		nemoshow_sequence_dispatch(trans->sequences[i], t, trans->serial);
	}

	for (i = 0; i < trans->ndones; i++) {
		if (trans->dones[i] != NULL)
			nemoshow_one_dirty(trans->dones[i], trans->dirties[i]);
	}

	if (trans->dispatch_frame != NULL) {
		trans->dispatch_frame(trans->userdata, time, t);
	}

	if (done != 0 && trans->dispatch_done != NULL)
		trans->dispatch_done(trans->userdata);

	return done;
}
