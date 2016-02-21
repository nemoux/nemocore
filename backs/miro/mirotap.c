#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <float.h>

#include <miroback.h>
#include <mirotap.h>
#include <talehelper.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct mirotap *nemoback_mirotap_create(struct miroback *miro)
{
	struct mirotap *tap;

	tap = (struct mirotap *)malloc(sizeof(struct mirotap));
	if (tap == NULL)
		return NULL;
	memset(tap, 0, sizeof(struct mirotap));

	tap->miro = miro;

	nemolist_insert(&miro->tap_list, &tap->link);

	nemosignal_init(&tap->destroy_signal);

	return tap;
}

void nemoback_mirotap_destroy(struct mirotap *tap)
{
	nemolist_remove(&tap->link);

	nemosignal_emit(&tap->destroy_signal, tap);

	if (tap->timer != NULL)
		nemotimer_destroy(tap->timer);

	free(tap);
}

static void nemoback_mirotap_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct mirotap *tap = (struct mirotap *)data;
	struct miroback *miro = tap->miro;
	struct showtransition *trans;
	struct showone *set0, *set1;
	uint32_t duration = 500;
	uint32_t delay = 100;
	int i;

	if (tap->state == MIROBACK_TAP_NORMAL_STATE) {
		struct showone *ones[] = {
			tap->one1,
			tap->one2,
			tap->one3,
			tap->one4
		};
		int nones = sizeof(ones) / sizeof(ones[0]);

		for (i = 0; i < nones; i++) {
			set0 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set0, ones[i]);
			nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);

			set1 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set1, ones[i]);
			nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);

			trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, duration, delay * i);
			nemoshow_transition_check_one(trans, ones[i]);
			nemoshow_transition_attach_sequence(trans,
					nemoshow_sequence_create_easy(miro->show,
						nemoshow_sequence_create_frame_easy(miro->show,
							0.7f, set0, NULL),
						nemoshow_sequence_create_frame_easy(miro->show,
							1.0f, set1, NULL),
						NULL));
			nemoshow_attach_transition(miro->show, trans);
		}
	} else {
		struct showone *ones[] = {
			tap->one4,
			tap->one3,
			tap->one2,
			tap->one1
		};
		int nones = sizeof(ones) / sizeof(ones[0]);

		for (i = 0; i < nones; i++) {
			set0 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set0, ones[i]);
			nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);

			set1 = nemoshow_sequence_create_set();
			nemoshow_sequence_set_source(set1, ones[i]);
			nemoshow_sequence_set_dattr(set1, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);

			trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, duration, delay * i);
			nemoshow_transition_check_one(trans, ones[i]);
			nemoshow_transition_attach_sequence(trans,
					nemoshow_sequence_create_easy(miro->show,
						nemoshow_sequence_create_frame_easy(miro->show,
							0.7f, set0, NULL),
						nemoshow_sequence_create_frame_easy(miro->show,
							1.0f, set1, NULL),
						NULL));
			nemoshow_attach_transition(miro->show, trans);
		}
	}

	nemotimer_set_timeout(timer, 1500);
}

int nemoback_mirotap_down(struct miroback *miro, struct mirotap *tap, double x, double y)
{
	struct showone *one;
	struct showone *blur;
	struct showtransition *trans0, *trans1;
	struct showone *sequence;
	struct showone *set0, *set1;
	struct nemotimer *timer;
	int attr;

	tap->x = x;
	tap->y = y;

	tap->blur = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(blur, "high", "solid", 0.0f);

	tap->one0 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, miro->tapsize);
	nemoshow_item_set_height(one, miro->tapsize);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, tap->blur);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, miro->tapsize / 2.0f, miro->tapsize / 2.0f);
	nemoshow_item_translate(one, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_scale(one, 0.0f, 0.0f);
	nemoshow_item_load_svg(one, NEMOUX_MIROBACK_RESOURCES "/touch-1.svg");

	tap->one1 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, miro->tapsize);
	nemoshow_item_set_height(one, miro->tapsize);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, tap->blur);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, miro->tapsize / 2.0f, miro->tapsize / 2.0f);
	nemoshow_item_translate(one, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_load_svg(one, NEMOUX_MIROBACK_RESOURCES "/touch-3-1.svg");
	nemoshow_item_set_alpha(one, 0.0f);

	tap->one2 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, miro->tapsize);
	nemoshow_item_set_height(one, miro->tapsize);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, tap->blur);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, miro->tapsize / 2.0f, miro->tapsize / 2.0f);
	nemoshow_item_translate(one, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_load_svg(one, NEMOUX_MIROBACK_RESOURCES "/touch-3-2.svg");
	nemoshow_item_set_alpha(one, 0.0f);

	tap->one3 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, miro->tapsize);
	nemoshow_item_set_height(one, miro->tapsize);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, tap->blur);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, miro->tapsize / 2.0f, miro->tapsize / 2.0f);
	nemoshow_item_translate(one, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_load_svg(one, NEMOUX_MIROBACK_RESOURCES "/touch-3-3.svg");
	nemoshow_item_set_alpha(one, 0.0f);

	tap->one4 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, miro->tapsize);
	nemoshow_item_set_height(one, miro->tapsize);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, tap->blur);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, miro->tapsize / 2.0f, miro->tapsize / 2.0f);
	nemoshow_item_translate(one, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_load_svg(one, NEMOUX_MIROBACK_RESOURCES "/touch-3-4.svg");
	nemoshow_item_set_alpha(one, 0.0f);

	tap->oner = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(miro->canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, miro->tapsize);
	nemoshow_item_set_height(one, miro->tapsize);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(one, tap->blur);
	nemoshow_item_set_alpha(one, 0.0f);
	nemoshow_item_set_tsr(one);
	nemoshow_item_pivot(one, miro->tapsize / 2.0f, miro->tapsize / 2.0f);
	nemoshow_item_translate(one, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_scale(one, 0.9f, 0.9f);
	nemoshow_item_load_svg(one, NEMOUX_MIROBACK_RESOURCES "/touch-2.svg");

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, tap->one0);
	nemoshow_sequence_set_dattr(set0, "sx", 1.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "sy", 1.0f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, tap->oner);
	nemoshow_sequence_set_dattr(set1, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);

	sequence = nemoshow_sequence_create_easy(miro->show,
			nemoshow_sequence_create_frame_easy(miro->show,
				1.0f, set0, set1, NULL),
			NULL);

	trans0 = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 800, 0);
	nemoshow_transition_check_one(trans0, tap->one0);
	nemoshow_transition_check_one(trans0, tap->oner);
	nemoshow_transition_attach_sequence(trans0, sequence);
	nemoshow_attach_transition(miro->show, trans0);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, tap->oner);
	attr = nemoshow_sequence_set_dattr(set0, "ro", 360.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_fix_dattr(set0, attr, 0.0f);

	sequence = nemoshow_sequence_create_easy(miro->show,
			nemoshow_sequence_create_frame_easy(miro->show,
				1.0f, set0, NULL),
			NULL);

	trans1 = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 3000, 0);
	nemoshow_transition_check_one(trans1, tap->oner);
	nemoshow_transition_attach_sequence(trans1, sequence);
	nemoshow_transition_set_repeat(trans1, 0);
	nemoshow_attach_transition_after(miro->show, trans0, trans1);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, tap->blur);
	nemoshow_sequence_set_dattr(set0, "r", 15.0f, NEMOSHOW_SHAPE_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, tap->blur);
	nemoshow_sequence_set_dattr(set1, "r", 3.0f, NEMOSHOW_SHAPE_DIRTY);

	sequence = nemoshow_sequence_create_easy(miro->show,
			nemoshow_sequence_create_frame_easy(miro->show,
				0.5f, set0, NULL),
			nemoshow_sequence_create_frame_easy(miro->show,
				1.0f, set1, NULL),
			NULL);

	trans1 = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 1200, 0);
	nemoshow_transition_check_one(trans1, tap->blur);
	nemoshow_transition_attach_sequence(trans1, sequence);
	nemoshow_transition_set_repeat(trans1, 0);
	nemoshow_attach_transition_after(miro->show, trans0, trans1);

	tap->timer = timer = nemotimer_create(miro->tool);
	nemotimer_set_callback(timer, nemoback_mirotap_dispatch_timer);
	nemotimer_set_userdata(timer, tap);
	nemotimer_set_timeout(timer, 1000);

	return 0;
}

int nemoback_mirotap_motion(struct miroback *miro, struct mirotap *tap, double x, double y)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1, *set2, *set3, *set4, *setr;
	struct mirotap *otap;
	int has_collision = 0;
	double dx, dy;

	tap->x = x;
	tap->y = y;

	nemoshow_item_translate(tap->one0, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_translate(tap->one1, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_translate(tap->one2, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_translate(tap->one3, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_translate(tap->one4, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);
	nemoshow_item_translate(tap->oner, x - miro->tapsize / 2.0f, y - miro->tapsize / 2.0f);

	nemolist_for_each(otap, &miro->tap_list, link) {
		if (otap == tap)
			continue;

		dx = otap->x - tap->x;
		dy = otap->y - tap->y;

		if (sqrtf(dx * dx + dy * dy) < miro->tapsize) {
			has_collision = 1;
			break;
		}
	}

	if (has_collision == 0 && tap->state != MIROBACK_TAP_NORMAL_STATE) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, tap->one0);
		nemoshow_sequence_set_cattr(set0, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, tap->one1);
		nemoshow_sequence_set_cattr(set1, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

		set2 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set2, tap->one2);
		nemoshow_sequence_set_cattr(set2, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

		set3 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set3, tap->one3);
		nemoshow_sequence_set_cattr(set3, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

		set4 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set4, tap->one4);
		nemoshow_sequence_set_cattr(set4, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

		setr = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(setr, tap->oner);
		nemoshow_sequence_set_cattr(setr, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(setr, "sx", 0.9f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(setr, "sy", 0.9f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, set1, set2, set3, set4, setr, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 500, 0);
		nemoshow_transition_check_one(trans, tap->one0);
		nemoshow_transition_check_one(trans, tap->one1);
		nemoshow_transition_check_one(trans, tap->one2);
		nemoshow_transition_check_one(trans, tap->one3);
		nemoshow_transition_check_one(trans, tap->one4);
		nemoshow_transition_check_one(trans, tap->oner);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);

		tap->state = MIROBACK_TAP_NORMAL_STATE;
	} else if (has_collision != 0 && tap->state != MIROBACK_TAP_COLLISION_STATE) {
		set0 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set0, tap->one0);
		nemoshow_sequence_set_cattr(set0, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);

		set1 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set1, tap->one1);
		nemoshow_sequence_set_cattr(set1, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);

		set2 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set2, tap->one2);
		nemoshow_sequence_set_cattr(set2, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);

		set3 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set3, tap->one3);
		nemoshow_sequence_set_cattr(set3, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);

		set4 = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(set4, tap->one4);
		nemoshow_sequence_set_cattr(set4, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);

		setr = nemoshow_sequence_create_set();
		nemoshow_sequence_set_source(setr, tap->oner);
		nemoshow_sequence_set_cattr(setr, "fill", 0xff, 0x8c, 0x32, 0xff, NEMOSHOW_STYLE_DIRTY);
		nemoshow_sequence_set_dattr(setr, "sx", 0.15f, NEMOSHOW_MATRIX_DIRTY);
		nemoshow_sequence_set_dattr(setr, "sy", 0.15f, NEMOSHOW_MATRIX_DIRTY);

		sequence = nemoshow_sequence_create_easy(miro->show,
				nemoshow_sequence_create_frame_easy(miro->show,
					1.0f, set0, set1, set2, set3, set4, setr, NULL),
				NULL);

		trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 500, 0);
		nemoshow_transition_check_one(trans, tap->one0);
		nemoshow_transition_check_one(trans, tap->one1);
		nemoshow_transition_check_one(trans, tap->one2);
		nemoshow_transition_check_one(trans, tap->one3);
		nemoshow_transition_check_one(trans, tap->one4);
		nemoshow_transition_check_one(trans, tap->oner);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(miro->show, trans);

		tap->state = MIROBACK_TAP_COLLISION_STATE;
	}

	return 0;
}

static void nemoback_mirotap_dispatch_destroy_done(void *data)
{
	struct mirotap *tap = (struct mirotap *)data;

	nemoshow_one_destroy(tap->blur);
	nemoshow_one_destroy(tap->one0);
	nemoshow_one_destroy(tap->one1);
	nemoshow_one_destroy(tap->one2);
	nemoshow_one_destroy(tap->one3);
	nemoshow_one_destroy(tap->one4);
	nemoshow_one_destroy(tap->oner);

	nemoback_mirotap_destroy(tap);
}

int nemoback_mirotap_up(struct miroback *miro, struct mirotap *tap, double x, double y)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0, *set1, *set2, *set3, *set4, *setr;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, tap->one0);
	nemoshow_sequence_set_dattr(set0, "sx", 0.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set0, "sy", 0.0f, NEMOSHOW_MATRIX_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, tap->one1);
	nemoshow_sequence_set_dattr(set1, "sx", 0.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set1, "sy", 0.0f, NEMOSHOW_MATRIX_DIRTY);

	set2 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set2, tap->one2);
	nemoshow_sequence_set_dattr(set2, "sx", 0.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set2, "sy", 0.0f, NEMOSHOW_MATRIX_DIRTY);

	set3 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set3, tap->one3);
	nemoshow_sequence_set_dattr(set3, "sx", 0.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set3, "sy", 0.0f, NEMOSHOW_MATRIX_DIRTY);

	set4 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set4, tap->one4);
	nemoshow_sequence_set_dattr(set4, "sx", 0.0f, NEMOSHOW_MATRIX_DIRTY);
	nemoshow_sequence_set_dattr(set4, "sy", 0.0f, NEMOSHOW_MATRIX_DIRTY);

	setr = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(setr, tap->oner);
	nemoshow_sequence_set_dattr(setr, "alpha", 0.0f, NEMOSHOW_STYLE_DIRTY);

	sequence = nemoshow_sequence_create_easy(miro->show,
			nemoshow_sequence_create_frame_easy(miro->show,
				1.0f, set0, set1, set2, set3, set4, setr, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 700, 0);
	nemoshow_transition_set_dispatch_done(trans, nemoback_mirotap_dispatch_destroy_done);
	nemoshow_transition_set_userdata(trans, tap);
	nemoshow_transition_check_one(trans, tap->one0);
	nemoshow_transition_check_one(trans, tap->one1);
	nemoshow_transition_check_one(trans, tap->one2);
	nemoshow_transition_check_one(trans, tap->one3);
	nemoshow_transition_check_one(trans, tap->one4);
	nemoshow_transition_check_one(trans, tap->oner);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(miro->show, trans);

	nemotimer_set_timeout(tap->timer, 0);

	return 0;
}
