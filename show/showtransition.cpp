#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showtransition.h>
#include <showsequence.h>
#include <nemobox.h>
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

	trans->sequences = (struct showone **)malloc(sizeof(struct showone *) * 4);
	trans->nsequences = 0;
	trans->ssequences = 4;

	trans->ease = NEMOSHOW_EASE(ease);

	trans->duration = duration;
	trans->delay = delay;
	trans->repeat = 1;

	return trans;
}

void nemoshow_transition_destroy(struct showtransition *trans)
{
	struct transitionsensor *sensor, *nsensor;

	nemolist_for_each_safe(sensor, nsensor, &trans->sensor_list, link) {
		nemolist_remove(&sensor->link);
		nemolist_remove(&sensor->listener.link);

		free(sensor);
	}

	nemolist_remove(&trans->link);
	nemolist_remove(&trans->sensor_list);

	free(trans->sequences);
	free(trans);
}

static void nemoshow_transition_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct transitionsensor *sensor = (struct transitionsensor *)container_of(listener, struct transitionsensor, listener);

	nemoshow_transition_destroy(sensor->transition);
}

void nemoshow_transition_check_one(struct showtransition *trans, struct showone *one)
{
	struct transitionsensor *sensor;

	sensor = (struct transitionsensor *)malloc(sizeof(struct transitionsensor));
	if (sensor != NULL) {
		sensor->transition = trans;

		sensor->listener.notify = nemoshow_transition_handle_destroy_signal;
		nemosignal_add(&one->destroy_signal, &sensor->listener);

		nemolist_insert_tail(&trans->sensor_list, &sensor->link);
	}
}

void nemoshow_transition_attach_sequence(struct showtransition *trans, struct showone *sequence)
{
	NEMOBOX_APPEND(trans->sequences, trans->ssequences, trans->nsequences, sequence);
}

void nemoshow_transition_attach_transition(struct showtransition *trans, struct showtransition *ntrans)
{
	NEMOBOX_APPEND(trans->transitions, trans->stransitions, trans->ntransitions, ntrans);
}

int nemoshow_transition_dispatch(struct showtransition *trans, uint32_t time)
{
	double t;
	int done = 0;
	int i;

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

	if (done == 0 && trans->dispatch_frame != NULL) {
		trans->dispatch_frame(trans->userdata, time, t);
	}

	return done;
}
