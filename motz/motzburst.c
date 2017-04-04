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
	struct nemotozz *tozz = motz->tozz;

	nemotozz_save(tozz);
	nemotozz_concat(tozz, burst->matrix);

	nemotozz_draw_circle(tozz, burst->style, 0.0f, 0.0f, burst->size / 2.0f);

	nemotozz_restore(tozz);
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
	nemomotz_set_flags(motz, NEMOMOTZ_REDRAW_FLAG);

	nemomotz_one_destroy(one);
}

static struct motzone *nemomotz_burst_contain(struct motzone *one, float x, float y)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	return NULL;
}

static void nemomotz_burst_update(struct motzone *one)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_BURST_TRANSFORM_DIRTY) != 0) {
		nemotozz_matrix_translate(burst->matrix, burst->tx, burst->ty);

		nemotozz_matrix_invert(burst->inverse, burst->matrix);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_BURST_COLOR_DIRTY) != 0)
		nemotozz_style_set_color(burst->style, burst->r, burst->g, burst->b, burst->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_BURST_STROKE_WIDTH_DIRTY) != 0)
		nemotozz_style_set_stroke_width(burst->style, burst->stroke_width);
}

static int nemomotz_burst_frame(struct motzone *one, uint32_t msecs)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	nemotozz_style_set_color(burst->style,
			random_get_double(64.0f, 255.0f),
			random_get_double(64.0f, 255.0f),
			random_get_double(64.0f, 255.0f),
			255.0f);

	return 1;
}

static void nemomotz_burst_destroy(struct motzone *one)
{
	struct motzburst *burst = NEMOMOTZ_BURST(one);

	nemomotz_one_finish(one);

	nemotozz_style_destroy(burst->style);
	nemotozz_matrix_destroy(burst->matrix);
	nemotozz_matrix_destroy(burst->inverse);

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
	one->frame = nemomotz_burst_frame;
	one->destroy = nemomotz_burst_destroy;

	burst->style = nemotozz_style_create();
	nemotozz_style_set_type(burst->style, NEMOTOZZ_STYLE_STROKE_AND_FILL_TYPE);

	burst->matrix = nemotozz_matrix_create();
	burst->inverse = nemotozz_matrix_create();

	return one;
}
