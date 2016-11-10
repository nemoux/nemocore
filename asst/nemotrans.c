#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotrans.h>

struct nemotrans *nemotrans_create(int type, uint32_t duration, uint32_t delay)
{
	struct nemotrans *trans;

	trans = (struct nemotrans *)malloc(sizeof(struct nemotrans));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct nemotrans));

	nemolist_init(&trans->link);
	nemolist_init(&trans->list);

	nemoease_set(&trans->ease, type);

	trans->duration = duration;
	trans->delay = delay;

	trans->stime = 0;
	trans->etime = 0;

	return trans;
}

void nemotrans_destroy(struct nemotrans *trans)
{
	struct transone *one, *next;

	nemolist_remove(&trans->link);

	nemolist_for_each_safe(one, next, &trans->list, link) {
		nemolist_remove(&one->link);

		free(one);
	}

	free(trans);
}

void nemotrans_ease_set_type(struct nemotrans *trans, int type)
{
	nemoease_set(&trans->ease, type);
}

void nemotrans_ease_set_bezier(struct nemotrans *trans, double x0, double y0, double x1, double y1)
{
	nemoease_set_cubic(&trans->ease, x0, y0, x1, y1);
}

int nemotrans_dispatch(struct nemotrans *trans, uint32_t time)
{
	struct transone *one;
	double t;
	int done = 0;

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
		t = nemoease_get(&trans->ease, time - trans->stime, trans->duration);
	}

	nemolist_for_each(one, &trans->list, link) {
		double v = (one->eattr - one->sattr) * t + one->sattr;

		if (one->is_double == 0)
			nemoattr_setf(&one->attr, v);
		else
			nemoattr_setd(&one->attr, v);
	}

	return done;
}

void nemotrans_set_float(struct nemotrans *trans, float *var, float value)
{
	struct transone *one;

	one = (struct transone *)malloc(sizeof(struct transone));
	one->sattr = *var;
	one->eattr = value;
	one->is_double = 0;

	nemoattr_setp(&one->attr, var);

	nemolist_insert_tail(&trans->list, &one->link);
}

void nemotrans_set_double(struct nemotrans *trans, double *var, double value)
{
	struct transone *one;

	one = (struct transone *)malloc(sizeof(struct transone));
	one->sattr = *var;
	one->eattr = value;
	one->is_double = 1;

	nemoattr_setp(&one->attr, var);

	nemolist_insert_tail(&trans->list, &one->link);
}
