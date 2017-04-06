#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzswirl.h>
#include <nemomisc.h>

static void nemomotz_swirl_draw(struct nemomotz *motz, struct motzone *one)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);
	struct nemotozz *tozz = motz->tozz;

	nemotozz_save(tozz);
	nemotozz_concat(tozz, swirl->matrix);

	nemotozz_draw_circle(tozz, swirl->style, swirl->x, swirl->y, swirl->size);

	nemotozz_restore(tozz);
}

static void nemomotz_swirl_dispatch_transition_update(struct nemotransition *trans, void *data, float t)
{
	struct motzone *one = (struct motzone *)data;
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);

	nemomotz_swirl_set_size(one, swirl->scale * (1.0f - t));
	nemomotz_swirl_set_y(one, t * swirl->scale);
	nemomotz_swirl_set_x(one, sin(swirl->frequence * t) * swirl->size);
}

static void nemomotz_swirl_dispatch_transition_done(struct nemotransition *trans, void *data)
{
	struct motzone *one = (struct motzone *)data;

	nemomotz_one_destroy(one);
}

static void nemomotz_swirl_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);
	struct nemotransition *trans;

	trans = nemotransition_create(8, NEMOEASE_LINEAR_TYPE, swirl->duration, swirl->delay);
	nemotransition_set_dispatch_update(trans, nemomotz_swirl_dispatch_transition_update);
	nemotransition_set_dispatch_done(trans, nemomotz_swirl_dispatch_transition_done);
	nemotransition_set_userdata(trans, one);

	nemomotz_transition_swirl_check_destroy(trans, one);
	nemomotz_attach_transition(motz, trans);
}

static void nemomotz_swirl_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_swirl_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static struct motzone *nemomotz_swirl_contain(struct motzone *one, float x, float y)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);

	return NULL;
}

static void nemomotz_swirl_update(struct motzone *one)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_SWIRL_TRANSFORM_DIRTY) != 0) {
		nemotozz_matrix_translate(swirl->matrix, swirl->tx, swirl->ty);

		nemotozz_matrix_invert(swirl->inverse, swirl->matrix);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_SWIRL_COLOR_DIRTY) != 0)
		nemotozz_style_set_color(swirl->style, swirl->r, swirl->g, swirl->b, swirl->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_SWIRL_STROKE_WIDTH_DIRTY) != 0)
		nemotozz_style_set_stroke_width(swirl->style, swirl->stroke_width);
}

static void nemomotz_swirl_destroy(struct motzone *one)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);

	nemomotz_one_finish(one);

	nemotozz_style_destroy(swirl->style);
	nemotozz_matrix_destroy(swirl->matrix);
	nemotozz_matrix_destroy(swirl->inverse);

	free(swirl);
}

struct motzone *nemomotz_swirl_create(void)
{
	struct motzswirl *swirl;
	struct motzone *one;

	swirl = (struct motzswirl *)malloc(sizeof(struct motzswirl));
	if (swirl == NULL)
		return NULL;
	memset(swirl, 0, sizeof(struct motzswirl));

	one = &swirl->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_swirl_draw;
	one->down = nemomotz_swirl_down;
	one->motion = nemomotz_swirl_motion;
	one->up = nemomotz_swirl_up;
	one->contain = nemomotz_swirl_contain;
	one->update = nemomotz_swirl_update;
	one->destroy = nemomotz_swirl_destroy;

	swirl->style = nemotozz_style_create();
	nemotozz_style_set_type(swirl->style, NEMOTOZZ_STYLE_STROKE_AND_FILL_TYPE);

	swirl->matrix = nemotozz_matrix_create();
	swirl->inverse = nemotozz_matrix_create();

	return one;
}
