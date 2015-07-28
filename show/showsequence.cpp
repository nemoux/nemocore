#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showsequence.h>
#include <nemoshow.h>
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
	one->destroy = nemoshow_sequence_destroy;

	nemoshow_one_prepare(one);

	sequence->frames = (struct showone **)malloc(sizeof(struct showone *) * 4);
	sequence->nframes = 0;
	sequence->sframes = 4;

	return one;
}

void nemoshow_sequence_destroy(struct showone *one)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);

	nemoshow_one_finish(one);

	free(sequence->frames);
	free(sequence);
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
	one->destroy = nemoshow_sequence_destroy_frame;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "t", &frame->t, sizeof(double));

	frame->sets = (struct showone **)malloc(sizeof(struct showone *) * 4);
	frame->nsets = 0;
	frame->ssets = 4;

	return one;
}

void nemoshow_sequence_destroy_frame(struct showone *one)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);

	nemoshow_one_finish(one);

	free(frame->sets);
	free(frame);
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
	one->destroy = nemoshow_sequence_destroy_set;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "src", set->src, NEMOSHOW_ID_MAX);

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
	struct nemoattr *attr;
	const char *name;
	int i, count;

	src = nemoshow_search_one(show, set->src);
	if (src == NULL)
		return 0;

	count = nemoobject_get_count(&one->object);

	for (i = 0; i < count; i++) {
		name = nemoobject_get_name(&one->object, i);

		if (strcmp(name, "id") == 0 || strcmp(name, "src") == 0)
			continue;

		attr = nemoobject_get(&src->object, name);
		if (attr == NULL)
			continue;
		set->tattrs[set->nattrs] = attr;
		set->eattrs[set->nattrs] = nemoobject_iget(&one->object, i);

		set->nattrs++;
	}

	return 1;
}

static void nemoshow_sequence_prepare_frame(struct showone *one)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	int i, j;

	for (i = 0; i < frame->nsets; i++) {
		set = NEMOSHOW_SET(frame->sets[i]);

		for (j = 0; j < set->nattrs; j++) {
			set->sattrs[j] = nemoattr_getd(set->tattrs[j]);
		}
	}
}

static void nemoshow_sequence_update_frame(struct showone *one, double s, double t)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	double dt = (t - s) / (frame->t - s);
	int i, j;

	for (i = 0; i < frame->nsets; i++) {
		set = NEMOSHOW_SET(frame->sets[i]);

		for (j = 0; j < set->nattrs; j++) {
			nemoattr_setd(set->tattrs[j],
					(nemoattr_getd(set->eattrs[j]) - set->sattrs[j]) * dt + set->sattrs[j]);
		}
	}
}

static void nemoshow_sequence_finish_frame(struct showone *one)
{
	struct showframe *frame = NEMOSHOW_FRAME(one);
	struct showset *set;
	int i, j;

	for (i = 0; i < frame->nsets; i++) {
		set = NEMOSHOW_SET(frame->sets[i]);

		for (j = 0; j < set->nattrs; j++) {
			nemoattr_setd(set->tattrs[j], nemoattr_getd(set->eattrs[j]));
		}
	}
}

void nemoshow_sequence_prepare(struct showone *one)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);

	sequence->t = 0.0f;
	sequence->iframe = 0;

	nemoshow_sequence_prepare_frame(sequence->frames[sequence->iframe]);
}

void nemoshow_sequence_update(struct showone *one, double t)
{
	struct showsequence *sequence = NEMOSHOW_SEQUENCE(one);
	struct showframe *frame = NEMOSHOW_FRAME(sequence->frames[sequence->iframe]);

	if (t >= 1.0f) {
		nemoshow_sequence_finish_frame(sequence->frames[sequence->iframe]);
	} else if (frame->t < t) {
		nemoshow_sequence_finish_frame(sequence->frames[sequence->iframe]);

		sequence->t = frame->t;
		sequence->iframe++;

		nemoshow_sequence_prepare_frame(sequence->frames[sequence->iframe]);
	} else {
		nemoshow_sequence_update_frame(sequence->frames[sequence->iframe], sequence->t, t);
	}
}
