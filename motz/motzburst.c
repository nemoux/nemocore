#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzburst.h>
#include <nemomisc.h>

static void nemomotz_burst_draw(struct nemomotz *motz, struct motzone *one)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);
	struct nemotoyz *toyz = motz->toyz;

	nemotoyz_save(toyz);
	nemotoyz_concat(toyz, burst->matrix);

	nemotoyz_draw_circle(toyz, burst->style, 0.0f, 0.0f, burst->size / 2.0f);

	nemotoyz_restore(toyz);
}

static void nemomotz_burst_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	nemomotz_burst_set_tx(one, x);
	nemomotz_burst_set_ty(one, y);
}

static void nemomotz_burst_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	nemomotz_burst_set_tx(one, x);
	nemomotz_burst_set_ty(one, y);
}

static void nemomotz_burst_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
	nemomotz_one_destroy(one);
}

static int nemomotz_burst_contain(struct motzone *one, float x, float y)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	return 0;
}

static void nemomotz_burst_update(struct motzone *one)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_BURST_TRANSFORM_DIRTY) != 0) {
		nemotoyz_matrix_translate(burst->matrix, burst->tx, burst->ty);

		nemotoyz_matrix_invert(burst->inverse, burst->matrix);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_BURST_COLOR_DIRTY) != 0)
		nemotoyz_style_set_color(burst->style, burst->r, burst->g, burst->b, burst->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_BURST_STROKE_WIDTH_DIRTY) != 0)
		nemotoyz_style_set_stroke_width(burst->style, burst->stroke_width);
}

static void nemomotz_burst_destroy(struct motzone *one)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	nemomotz_one_finish(one);

	nemotoyz_style_destroy(burst->style);
	nemotoyz_matrix_destroy(burst->matrix);
	nemotoyz_matrix_destroy(burst->inverse);

	free(burst);
}

struct motzone *nemomotz_burst_create(void)
{
	struct motzburst *burst;
	struct motzone *one;

	burst = (struct motzburst *)malloc(sizeof(struct motzburst));
	if (burst == NULL)
		return NULL;
	memset(burst, 0, sizeof(struct motzburst));

	one = &burst->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_burst_draw;
	one->down = nemomotz_burst_down;
	one->motion = nemomotz_burst_motion;
	one->up = nemomotz_burst_up;
	one->contain = nemomotz_burst_contain;
	one->update = nemomotz_burst_update;
	one->destroy = nemomotz_burst_destroy;

	burst->style = nemotoyz_style_create();
	nemotoyz_style_set_type(burst->style, NEMOTOYZ_STYLE_STROKE_AND_FILL_TYPE);

	burst->matrix = nemotoyz_matrix_create();
	burst->inverse = nemotoyz_matrix_create();

	return one;
}
