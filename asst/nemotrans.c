#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotrans.h>
#include <nemomisc.h>

#define NEMOTRANS_ONE_TARGETS_MAX			(8)

struct transone {
	struct nemoattr attr;
	int is_double;

	double timings[NEMOTRANS_ONE_TARGETS_MAX];
	double targets[NEMOTRANS_ONE_TARGETS_MAX];
	int count;
};

struct transgroup *nemotrans_group_create(void)
{
	struct transgroup *group;

	group = (struct transgroup *)malloc(sizeof(struct transgroup));
	if (group == NULL)
		return NULL;
	memset(group, 0, sizeof(struct transgroup));

	nemolist_init(&group->list);

	return group;
}

void nemotrans_group_destroy(struct transgroup *group)
{
	free(group);
}

void nemotrans_group_attach_trans(struct transgroup *group, struct nemotrans *trans)
{
	struct nemotrans *strans;
	struct transone *one, *sone;
	int i, j;

	for (i = 0; i < trans->nones; i++) {
		one = trans->ones[i];
		if (one != NULL) {
			nemolist_for_each(strans, &group->list, link) {
				for (j = 0; j < strans->nones; j++) {
					sone = strans->ones[j];
					if (sone != NULL) {
						if (nemoattr_getp(&one->attr) == nemoattr_getp(&sone->attr)) {
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

void nemotrans_group_detach_trans(struct transgroup *group, struct nemotrans *trans)
{
	nemolist_remove(&trans->link);
}

void nemotrans_group_ready(struct transgroup *group, uint32_t msecs)
{
	struct nemotrans *trans;

	nemolist_for_each(trans, &group->list, link) {
		nemotrans_ready(trans, msecs);
	}
}

void nemotrans_group_dispatch(struct transgroup *group, uint32_t msecs)
{
	struct nemotrans *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		if (nemotrans_dispatch(trans, msecs) != 0)
			nemotrans_destroy(trans);
	}
}

struct nemotrans *nemotrans_group_get_last_one(struct transgroup *group, void *var)
{
	struct nemotrans *ltrans = NULL;
	struct nemotrans *trans;
	struct transone *one;
	int i;

	nemolist_for_each(trans, &group->list, link) {
		for (i = 0; i < trans->nones; i++) {
			one = trans->ones[i];
			if (one != NULL) {
				if (nemoattr_getp(&one->attr) == var) {
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

struct nemotrans *nemotrans_group_get_last_tag(struct transgroup *group, uint32_t tag)
{
	struct nemotrans *ltrans = NULL;
	struct nemotrans *trans;

	nemolist_for_each(trans, &group->list, link) {
		if (trans->tag == tag) {
			if (ltrans == NULL || ltrans->etime < trans->etime)
				ltrans = trans;
		}
	}

	return ltrans;
}

struct nemotrans *nemotrans_group_get_last_all(struct transgroup *group)
{
	struct nemotrans *ltrans = NULL;
	struct nemotrans *trans;

	nemolist_for_each(trans, &group->list, link) {
		if (ltrans == NULL || ltrans->etime < trans->etime)
			ltrans = trans;
	}

	return ltrans;
}

void nemotrans_group_revoke_one(struct transgroup *group, void *var)
{
	struct nemotrans *trans;
	struct transone *one;
	int i;

	nemolist_for_each(trans, &group->list, link) {
		for (i = 0; i < trans->nones; i++) {
			one = trans->ones[i];
			if (one != NULL && nemoattr_getp(&one->attr) == var) {
				trans->ones[i] = NULL;
				free(one);
				break;
			}
		}
	}
}

void nemotrans_group_revoke_tag(struct transgroup *group, uint32_t tag)
{
	struct nemotrans *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		if (trans->tag == tag)
			nemotrans_destroy(trans);
	}
}

void nemotrans_group_revoke_all(struct transgroup *group)
{
	struct nemotrans *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &group->list, link) {
		nemotrans_destroy(trans);
	}
}

struct nemotrans *nemotrans_create(int max, int type, uint32_t duration, uint32_t delay)
{
	struct nemotrans *trans;

	trans = (struct nemotrans *)malloc(sizeof(struct nemotrans));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct nemotrans));

	trans->ones = (struct transone **)malloc(sizeof(struct transone *) * max);
	if (trans->ones == NULL)
		goto err1;
	memset(trans->ones, 0, sizeof(struct transone *) * max);

	trans->sones = max;
	trans->nones = 0;

	nemolist_init(&trans->link);

	nemoease_set(&trans->ease, type);

	trans->duration = duration;
	trans->delay = delay;

	trans->stime = 0;
	trans->etime = 0;

	return trans;

err1:
	free(trans);

	return NULL;
}

void nemotrans_destroy(struct nemotrans *trans)
{
	int i;

	nemolist_remove(&trans->link);

	for (i = 0; i < trans->nones; i++) {
		if (trans->ones[i] != NULL)
			free(trans->ones[i]);
	}

	free(trans->ones);
	free(trans);
}

void nemotrans_set_tag(struct nemotrans *trans, uint32_t tag)
{
	trans->tag = tag;
}

void nemotrans_ease_set_type(struct nemotrans *trans, int type)
{
	nemoease_set(&trans->ease, type);
}

void nemotrans_ease_set_bezier(struct nemotrans *trans, double x0, double y0, double x1, double y1)
{
	nemoease_set_cubic(&trans->ease, x0, y0, x1, y1);
}

int nemotrans_ready(struct nemotrans *trans, uint32_t msecs)
{
	if (trans->stime == 0) {
		trans->stime = msecs + trans->delay;
		trans->etime = msecs + trans->delay + trans->duration;
	}

	return 0;
}

int nemotrans_dispatch(struct nemotrans *trans, uint32_t msecs)
{
	struct transone *one;
	double t, u, v;
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

			if (one->is_double == 0)
				nemoattr_setf(&one->attr, v);
			else
				nemoattr_setd(&one->attr, v);
		}
	}

	if (trans->dispatch_update != NULL)
		trans->dispatch_update(trans, trans->data, t);

	if (trans->etime <= msecs && trans->dispatch_done != NULL)
		trans->dispatch_done(trans, trans->data);

	return trans->etime <= msecs;
}

void nemotrans_set_float(struct nemotrans *trans, int index, float *var)
{
	struct transone *one;

	trans->ones[index] = one = (struct transone *)malloc(sizeof(struct transone));
	one->count = 0;
	one->is_double = 0;

	nemoattr_setp(&one->attr, var);

	if (var != NULL)
		nemotrans_set_target(trans, index, 0.0f, *var);

	trans->nones = MAX(trans->nones, index + 1);
}

void nemotrans_set_double(struct nemotrans *trans, int index, double *var)
{
	struct transone *one;

	trans->ones[index] = one = (struct transone *)malloc(sizeof(struct transone));
	one->count = 0;
	one->is_double = 1;

	nemoattr_setp(&one->attr, var);

	if (var != NULL)
		nemotrans_set_target(trans, index, 0.0f, *var);

	trans->nones = MAX(trans->nones, index + 1);
}

float nemotrans_get_float(struct nemotrans *trans, int index)
{
	return nemoattr_getf(&trans->ones[index]->attr);
}

double nemotrans_get_double(struct nemotrans *trans, int index)
{
	return nemoattr_getd(&trans->ones[index]->attr);
}

void nemotrans_set_target(struct nemotrans *trans, int index, double t, double v)
{
	struct transone *one = trans->ones[index];
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

void nemotrans_set_dispatch_update(struct nemotrans *trans, nemotrans_dispatch_update_t dispatch)
{
	trans->dispatch_update = dispatch;
}

void nemotrans_set_dispatch_done(struct nemotrans *trans, nemotrans_dispatch_done_t dispatch)
{
	trans->dispatch_done = dispatch;
}

void nemotrans_set_userdata(struct nemotrans *trans, void *data)
{
	trans->data = data;
}
