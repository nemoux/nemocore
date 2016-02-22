#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>
#include <math.h>

#include <edgeroll.h>
#include <edgeback.h>
#include <edgemisc.h>
#include <nemoenvs.h>
#include <showhelper.h>
#include <nemotimer.h>
#include <nemolog.h>
#include <nemomisc.h>

struct edgeroll *nemoback_edgeroll_create(struct edgeback *edge, int site)
{
	struct edgeroll *roll;

	roll = (struct edgeroll *)malloc(sizeof(struct edgeroll));
	if (roll == NULL)
		return NULL;
	memset(roll, 0, sizeof(struct edgeroll));

	roll->edge = edge;

	roll->site = site;

	roll->groupidx = -1;
	roll->actionidx = -1;

	roll->serial = ++edge->roll_serial;

	nemosignal_init(&roll->destroy_signal);

	return roll;
}

void nemoback_edgeroll_destroy(struct edgeroll *roll)
{
	int i;

	nemosignal_emit(&roll->destroy_signal, roll);

	if (roll->timer != NULL)
		nemotimer_destroy(roll->timer);

	for (i = 0; i < roll->ngroups; i++) {
		nemoshow_one_destroy(roll->grouprings[i]);

		if (roll->groups[i] != NULL)
			nemoshow_one_destroy(roll->groups[i]);
	}

	free(roll);
}

static void nemoback_edgeroll_dispatch_destroy_done(void *data)
{
	struct edgeroll *roll = (struct edgeroll *)data;

	if (nemoback_edgeroll_unreference(roll) == 0)
		nemoback_edgeroll_destroy(roll);
}

static void nemoback_edgeroll_show_rings(struct edgeback *edge, struct edgeroll *roll, uint32_t ngroups)
{
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = roll->ngroups; i < ngroups; i++) {
		const char *ring = nemoenvs_get_group_ring(edge->envs, i);

		roll->grouprings[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
		nemoshow_one_attach(edge->canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, edge->rollsize);
		nemoshow_item_set_height(one, edge->rollsize);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_filter(one, NEMOSHOW_SOLID_SMALL_BLUR);
		nemoshow_item_set_tsr(one);
		nemoshow_item_pivot(one, edge->rollsize / 2.0f, edge->rollsize / 2.0f);
		nemoshow_item_scale(one, 0.0f, 0.0f);
		nemoshow_item_rotate(one, roll->r);
		nemoshow_item_load_svg(one, ring);

		if (roll->site == EDGEBACK_TOP_SITE) {
			nemoshow_item_translate(one,
					(roll->x1 + roll->x0) / 2.0f - edge->rollsize / 2.0f,
					(i + 0) * edge->rollsize);
		} else if (roll->site == EDGEBACK_BOTTOM_SITE) {
			nemoshow_item_translate(one,
					(roll->x1 + roll->x0) / 2.0f - edge->rollsize / 2.0f,
					edge->height - (i + 1) * edge->rollsize);
		} else if (roll->site == EDGEBACK_LEFT_SITE) {
			nemoshow_item_translate(one,
					(i + 0) * edge->rollsize,
					(roll->y1 + roll->y0) / 2.0f - edge->rollsize / 2.0f);
		} else if (roll->site == EDGEBACK_RIGHT_SITE) {
			nemoshow_item_translate(one,
					edge->width - (i + 1) * edge->rollsize,
					(roll->y1 + roll->y0) / 2.0f - edge->rollsize / 2.0f);
		}

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, one);
		nemoshow_sequence_set_dattr(set0, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 700, 0);
		nemoshow_transition_check_one(trans, one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);
	}

	roll->ngroups = i;
}

static void nemoback_edgeroll_hide_rings(struct edgeback *edge, struct edgeroll *roll, int needs_destroy)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i < roll->ngroups; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, roll->grouprings[i]);
		nemoshow_sequence_set_dattr(set0, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sx", 0.3f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 0.3f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 700, 0);
		if (needs_destroy != 0) {
			nemoback_edgeroll_reference(roll);

			nemoshow_transition_set_dispatch_done(trans, nemoback_edgeroll_dispatch_destroy_done);
			nemoshow_transition_set_userdata(trans, roll);
		}
		nemoshow_transition_check_one(trans, roll->grouprings[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);
	}
}

static void nemoback_edgeroll_show_groups(struct edgeback *edge, struct edgeroll *roll)
{
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i < roll->ngroups; i++) {
		const char *icon = nemoenvs_get_group_icon(edge->envs, i);

		roll->groups[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
		nemoshow_one_attach(edge->canvas, one);
		nemoshow_one_set_tag(one, NEMOSHOW_ONE_TAGGROUP(1000 + i, roll->serial));
		nemoshow_one_set_userdata(one, roll);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, edge->rollsize);
		nemoshow_item_set_height(one, edge->rollsize);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_filter(one, NEMOSHOW_SOLID_SMALL_BLUR);
		nemoshow_item_set_tsr(one);
		nemoshow_item_pivot(one, edge->rollsize / 2.0f, edge->rollsize / 2.0f);
		nemoshow_item_scale(one, 0.7f, 0.7f);
		nemoshow_item_rotate(one, roll->r);
		nemoshow_item_load_svg(one, icon);
		nemoshow_item_set_alpha(one, 0.0f);

		if (roll->site == EDGEBACK_TOP_SITE) {
			nemoshow_item_translate(one,
					(roll->x1 + roll->x0) / 2.0f - edge->rollsize / 2.0f,
					(i + 0) * edge->rollsize);
		} else if (roll->site == EDGEBACK_BOTTOM_SITE) {
			nemoshow_item_translate(one,
					(roll->x1 + roll->x0) / 2.0f - edge->rollsize / 2.0f,
					edge->height - (i + 1) * edge->rollsize);
		} else if (roll->site == EDGEBACK_LEFT_SITE) {
			nemoshow_item_translate(one,
					(i + 0) * edge->rollsize,
					(roll->y1 + roll->y0) / 2.0f - edge->rollsize / 2.0f);
		} else if (roll->site == EDGEBACK_RIGHT_SITE) {
			nemoshow_item_translate(one,
					edge->width - (i + 1) * edge->rollsize,
					(roll->y1 + roll->y0) / 2.0f - edge->rollsize / 2.0f);
		}

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, one);
		nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 700, 300);
		nemoshow_transition_check_one(trans, one);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);
	}
}

static void nemoback_edgeroll_hide_groups(struct edgeback *edge, struct edgeroll *roll, int needs_destroy)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	int i;

	for (i = 0; i < roll->ngroups; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, roll->groups[i]);
		nemoshow_sequence_set_dattr(set0, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sx", 0.7f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 0.7f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 700, 0);
		if (needs_destroy != 0) {
			nemoback_edgeroll_reference(roll);

			nemoshow_transition_set_dispatch_done(trans, nemoback_edgeroll_dispatch_destroy_done);
			nemoshow_transition_set_userdata(trans, roll);
		}
		nemoshow_transition_check_one(trans, roll->groups[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);
	}
}

int nemoback_edgeroll_shutdown(struct edgeback *edge, struct edgeroll *roll)
{
	nemoback_edgeroll_deactivate_group(edge, roll);

	nemoback_edgeroll_hide_rings(edge, roll, 1);
	nemoback_edgeroll_hide_groups(edge, roll, 1);

	nemoshow_dispatch_frame(edge->show);

	nemotimer_set_timeout(roll->timer, 0);

	roll->state = EDGEBACK_ROLL_DONE_STATE;

	return 0;
}

int nemoback_edgeroll_down(struct edgeback *edge, struct edgeroll *roll, double x, double y)
{
	if (roll->site == EDGEBACK_TOP_SITE) {
		roll->x0 = x;
		roll->y0 = 0.0f;
		roll->x1 = roll->x0;
		roll->y1 = y;

		roll->r = 180.0f;
	} else if (roll->site == EDGEBACK_BOTTOM_SITE) {
		roll->x0 = x;
		roll->y0 = y;
		roll->x1 = roll->x0;
		roll->y1 = edge->height;

		roll->r = 0.0f;
	} else if (roll->site == EDGEBACK_LEFT_SITE) {
		roll->x0 = 0.0f;
		roll->y0 = y;
		roll->x1 = x;
		roll->y1 = roll->y0;

		roll->r = 90.0f;
	} else if (roll->site == EDGEBACK_RIGHT_SITE) {
		roll->x0 = x;
		roll->y0 = y;
		roll->x1 = edge->width;
		roll->y1 = roll->y0;

		roll->r = -90.0f;
	}

	return 0;
}

static void nemoback_edgeroll_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct edgeroll *roll = (struct edgeroll *)data;
	struct edgeback *edge = roll->edge;

	nemoback_edgeroll_shutdown(edge, roll);
}

int nemoback_edgeroll_motion(struct edgeback *edge, struct edgeroll *roll, double x, double y)
{
	if (roll->state == EDGEBACK_ROLL_READY_STATE) {
		double size;

		if (roll->site == EDGEBACK_TOP_SITE) {
			roll->y1 = y;

			size = roll->y1 - roll->y0;
		} else if (roll->site == EDGEBACK_BOTTOM_SITE) {
			roll->y0 = y;

			size = roll->y1 - roll->y0;
		} else if (roll->site == EDGEBACK_LEFT_SITE) {
			roll->x1 = x;

			size = roll->x1 - roll->x0;
		} else if (roll->site == EDGEBACK_RIGHT_SITE) {
			roll->x0 = x;

			size = roll->x1 - roll->x0;
		}

		nemoback_edgeroll_show_rings(edge, roll, MIN(floor(size / edge->rollsize) + 1, nemoenvs_get_groups_count(edge->envs)));

		if (roll->ngroups >= nemoenvs_get_groups_count(edge->envs)) {
			struct nemotimer *timer;

			roll->timer = timer = nemotimer_create(edge->tool);
			nemotimer_set_callback(timer, nemoback_edgeroll_dispatch_timer);
			nemotimer_set_userdata(timer, roll);
			nemotimer_set_timeout(timer, edge->rolltimeout);

			roll->state = EDGEBACK_ROLL_ACTIVE_STATE;

			nemoback_edgeroll_show_groups(edge, roll);
		}
	} else if (roll->state == EDGEBACK_ROLL_ACTIVE_STATE) {
	}

	return 0;
}

int nemoback_edgeroll_up(struct edgeback *edge, struct edgeroll *roll, double x, double y)
{
	if (roll->state == EDGEBACK_ROLL_READY_STATE) {
		nemoback_edgeroll_hide_rings(edge, roll, 1);
	}

	return 0;
}

int nemoback_edgeroll_activate_group(struct edgeback *edge, struct edgeroll *roll, uint32_t group)
{
	struct showone *one;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1;
	double size;
	double x, y;
	int dd = group % 2 == 0 ? -1 : 1;
	int dx = 0, dy = 0;
	int i;

	if (roll->state != EDGEBACK_ROLL_ACTIVE_STATE)
		return 0;

	size = nemoenvs_get_actions_count(edge->envs, group) * edge->rollsize;

	if (roll->site == EDGEBACK_TOP_SITE) {
		x = (roll->x1 + roll->x0) / 2.0f - edge->rollsize / 2.0f;
		y = (group + 0) * edge->rollsize;

		dx = -1 * dd;
	} else if (roll->site == EDGEBACK_BOTTOM_SITE) {
		x = (roll->x1 + roll->x0) / 2.0f - edge->rollsize / 2.0f;
		y = edge->height - (group + 1) * edge->rollsize;

		dx = 1 * dd;
	} else if (roll->site == EDGEBACK_LEFT_SITE) {
		x = (group + 0) * edge->rollsize;
		y = (roll->y1 + roll->y0) / 2.0f - edge->rollsize / 2.0f;

		dy = 1 * dd;
	} else if (roll->site == EDGEBACK_RIGHT_SITE) {
		x = edge->width - (group + 1) * edge->rollsize;
		y = (roll->y1 + roll->y0) / 2.0f - edge->rollsize / 2.0f;

		dy = -1 * dd;
	}

	if (dx > 0 && x + edge->rollsize / 2.0f > edge->width - size)
		dx *= -1;
	if (dx < 0 && x + edge->rollsize / 2.0f < size)
		dx *= -1;
	if (dy > 0 && y + edge->rollsize / 2.0f > edge->height - size)
		dy *= -1;
	if (dy < 0 && y + edge->rollsize / 2.0f < size)
		dy *= -1;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, roll->grouprings[group]);
	nemoshow_sequence_set_dattr(set0, "sx", 0.8f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "sy", 0.8f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, roll->groups[group]);
	nemoshow_sequence_set_dattr(set1, "sx", 0.8f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set1, "sy", 0.8f, NEMOSHOW_MATRIX_DIRTY);

	sequence = nemoshow_sequence_create_easy(edge->show,
			nemoshow_sequence_create_frame_easy(edge->show,
				1.0f, set0, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 500, 0);
	nemoshow_transition_check_one(trans, roll->grouprings[group]);
	nemoshow_transition_check_one(trans, roll->groups[group]);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(edge->show, trans);

	for (i = 0; i < nemoenvs_get_actions_count(edge->envs, group); i++) {
		const char *ring = nemoenvs_get_action_ring(edge->envs, group, i);
		const char *icon = nemoenvs_get_action_icon(edge->envs, group, i);

		roll->actionrings[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
		nemoshow_one_attach(edge->canvas, one);
		nemoshow_one_set_tag(one, NEMOSHOW_ONE_TAGGROUP(2000 + i, roll->serial));
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, edge->rollsize);
		nemoshow_item_set_height(one, edge->rollsize);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_filter(one, NEMOSHOW_SOLID_SMALL_BLUR);
		nemoshow_item_set_tsr(one);
		nemoshow_item_pivot(one, edge->rollsize / 2.0f, edge->rollsize / 2.0f);
		nemoshow_item_scale(one, 0.3f, 0.3f);
		nemoshow_item_rotate(one, roll->r);
		nemoshow_item_load_svg(one, ring);
		nemoshow_item_set_alpha(one, 0.0f);

		nemoshow_item_translate(one,
				x + (i + 1) * edge->rollsize * dx,
				y + (i + 1) * edge->rollsize * dy);

		roll->actions[i] = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
		nemoshow_one_attach(edge->canvas, one);
		nemoshow_one_set_tag(one, NEMOSHOW_ONE_TAGGROUP(2000 + i, roll->serial));
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, edge->rollsize);
		nemoshow_item_set_height(one, edge->rollsize);
		nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
		nemoshow_item_set_filter(one, NEMOSHOW_SOLID_SMALL_BLUR);
		nemoshow_item_set_tsr(one);
		nemoshow_item_pivot(one, edge->rollsize / 2.0f, edge->rollsize / 2.0f);
		nemoshow_item_scale(one, 0.6f, 0.1f);
		nemoshow_item_rotate(one, roll->r);
		nemoshow_item_load_svg(one, icon);
		nemoshow_item_set_alpha(one, 0.0f);

		nemoshow_item_translate(one,
				x + (i + 1) * edge->rollsize * dx,
				y + (i + 1) * edge->rollsize * dy);

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, roll->actionrings[i]);
		nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 500, i * 100);
		nemoshow_transition_check_one(trans, roll->actionrings[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);

		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, roll->actions[i]);
		nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sx", 0.8f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 0.8f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 300, i * 100 + 200);
		nemoshow_transition_check_one(trans, roll->actions[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);

		nemoshow_transition_dispatch_rotate_easy(edge->show, roll->actionrings[i], NEMOSHOW_LINEAR_EASE, 5000, 0, 0);
	}

	nemoshow_dispatch_frame(edge->show);

	nemotimer_set_timeout(roll->timer, 0);

	roll->groupidx = group;
	roll->nactions = i;

	return 0;
}

int nemoback_edgeroll_deactivate_group(struct edgeback *edge, struct edgeroll *roll)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1;
	int group = roll->groupidx;
	int i;

	if (group < 0)
		return 0;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, roll->grouprings[group]);
	nemoshow_sequence_set_dattr(set0, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, roll->groups[group]);
	nemoshow_sequence_set_dattr(set1, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set1, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

	sequence = nemoshow_sequence_create_easy(edge->show,
			nemoshow_sequence_create_frame_easy(edge->show,
				1.0f, set0, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
	nemoshow_transition_check_one(trans, roll->grouprings[group]);
	nemoshow_transition_check_one(trans, roll->groups[group]);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(edge->show, trans);

	for (i = 0; i < roll->nactions; i++) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, roll->actionrings[i]);
		nemoshow_sequence_set_dattr(set0, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sx", 0.4f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set0, "sy", 0.4f, NEMOSHOW_MATRIX_DIRTY);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, roll->actions[i]);
		nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(set1, "sx", 0.2f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(set1, "sy", 0.2f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(edge->show,
				nemoshow_sequence_create_frame_easy(edge->show,
					1.0f, set0, set1, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 500, 0);
		nemoshow_transition_check_one(trans, roll->actionrings[i]);
		nemoshow_transition_check_one(trans, roll->actions[i]);
		nemoshow_transition_destroy_one(trans, roll->actionrings[i]);
		nemoshow_transition_destroy_one(trans, roll->actions[i]);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(edge->show, trans);
	}

	nemoshow_dispatch_frame(edge->show);

	nemotimer_set_timeout(roll->timer, edge->rolltimeout);

	roll->groupidx = -1;
	roll->actionidx = -1;

	return 0;
}

int nemoback_edgeroll_activate_action(struct edgeback *edge, struct edgeroll *roll, uint32_t action)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1;

	if (roll->state != EDGEBACK_ROLL_ACTIVE_STATE)
		return 0;

	if (action < 0 || action >= roll->nactions)
		return 0;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, roll->actionrings[action]);
	nemoshow_sequence_set_dattr(set0, "sx", 0.8f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "sy", 0.8f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, roll->actions[action]);
	nemoshow_sequence_set_dattr(set1, "sx", 0.6f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set1, "sy", 0.6f, NEMOSHOW_MATRIX_DIRTY);

	sequence = nemoshow_sequence_create_easy(edge->show,
			nemoshow_sequence_create_frame_easy(edge->show,
				1.0f, set0, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 300, 0);
	nemoshow_transition_check_one(trans, roll->actionrings[action]);
	nemoshow_transition_check_one(trans, roll->actions[action]);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(edge->show, trans);

	nemoshow_dispatch_frame(edge->show);

	roll->actionidx = action;

	return 0;
}

int nemoback_edgeroll_deactivate_action(struct edgeback *edge, struct edgeroll *roll)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1;
	int action = roll->actionidx;

	if (action < 0)
		return 0;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, roll->actionrings[action]);
	nemoshow_sequence_set_dattr(set0, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, roll->actions[action]);
	nemoshow_sequence_set_dattr(set1, "sx", 0.8f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set1, "sy", 0.8f, NEMOSHOW_MATRIX_DIRTY);

	sequence = nemoshow_sequence_create_easy(edge->show,
			nemoshow_sequence_create_frame_easy(edge->show,
				1.0f, set0, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 300, 0);
	nemoshow_transition_check_one(trans, roll->actionrings[action]);
	nemoshow_transition_check_one(trans, roll->actions[action]);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(edge->show, trans);

	nemoshow_dispatch_frame(edge->show);

	roll->actionidx = -1;

	return 0;
}
