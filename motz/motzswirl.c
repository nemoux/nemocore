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
	struct nemotoyz *toyz = motz->toyz;

	nemotoyz_save(toyz);
	nemotoyz_concat(toyz, swirl->matrix);

	nemotoyz_draw_circle(toyz, swirl->style, swirl->x, swirl->y, swirl->size);

	nemotoyz_restore(toyz);
}

static void nemomotz_swirl_dispatch_transition_update(struct motztransition *trans, void *data, float t)
{
	struct motzone *one = (struct motzone *)data;
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);

	nemomotz_swirl_set_size(one, swirl->scale * (1.0f - t));
	nemomotz_swirl_set_y(one, t * swirl->scale);
	nemomotz_swirl_set_x(one, sin(swirl->frequence * t) * swirl->size);
}

static void nemomotz_swirl_dispatch_transition_done(struct motztransition *trans, void *data)
{
	struct motzone *one = (struct motzone *)data;

	nemomotz_one_destroy(one);
}

static void nemomotz_swirl_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);
	struct motztransition *trans;

	trans = nemomotz_transition_create(8, NEMOEASE_LINEAR_TYPE, swirl->duration, swirl->delay);
	nemomotz_transition_set_dispatch_update(trans, nemomotz_swirl_dispatch_transition_update);
	nemomotz_transition_set_dispatch_done(trans, nemomotz_swirl_dispatch_transition_done);
	nemomotz_transition_set_userdata(trans, one);
	nemomotz_transition_check_one(trans, one);
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
		nemotoyz_matrix_translate(swirl->matrix, swirl->tx, swirl->ty);

		nemotoyz_matrix_invert(swirl->inverse, swirl->matrix);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_SWIRL_COLOR_DIRTY) != 0)
		nemotoyz_style_set_color(swirl->style, swirl->r, swirl->g, swirl->b, swirl->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_SWIRL_STROKE_WIDTH_DIRTY) != 0)
		nemotoyz_style_set_stroke_width(swirl->style, swirl->stroke_width);
}

static void nemomotz_swirl_destroy(struct motzone *one)
{
	struct motzswirl *swirl = NEMOMOTZ_SWIRL(one);

	nemomotz_one_finish(one);

	nemotoyz_style_destroy(swirl->style);
	nemotoyz_matrix_destroy(swirl->matrix);
	nemotoyz_matrix_destroy(swirl->inverse);

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

	nemomotz_one_prepare(one, sizeof(struct motzswirl));

	one->draw = nemomotz_swirl_draw;
	one->down = nemomotz_swirl_down;
	one->motion = nemomotz_swirl_motion;
	one->up = nemomotz_swirl_up;
	one->contain = nemomotz_swirl_contain;
	one->update = nemomotz_swirl_update;
	one->destroy = nemomotz_swirl_destroy;

	swirl->style = nemotoyz_style_create();
	nemotoyz_style_set_type(swirl->style, NEMOTOYZ_STYLE_STROKE_AND_FILL_TYPE);

	swirl->matrix = nemotoyz_matrix_create();
	swirl->inverse = nemotoyz_matrix_create();

	return one;
}
