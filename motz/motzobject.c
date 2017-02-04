#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzobject.h>
#include <nemomisc.h>

static void nemomotz_object_draw_line(struct nemomotz *motz, struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);

	nemotoyz_draw_line(motz->toyz,
			object->style,
			object->x,
			object->y,
			object->w,
			object->h);
}

static void nemomotz_object_draw_rect(struct nemomotz *motz, struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);

	nemotoyz_draw_rect(motz->toyz,
			object->style,
			object->x,
			object->y,
			object->w,
			object->h);
}

static void nemomotz_object_draw_round_rect(struct nemomotz *motz, struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);
}

static void nemomotz_object_draw_circle(struct nemomotz *motz, struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);

	nemotoyz_draw_circle(motz->toyz,
			object->style,
			object->x,
			object->y,
			object->radius);
}

static void nemomotz_object_draw_arc(struct nemomotz *motz, struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);
}

static void nemomotz_object_draw(struct nemomotz *motz, struct motzone *one)
{
	static nemomotz_one_draw_t draws[NEMOMOTZ_OBJECT_LAST_SHAPE] = {
		nemomotz_object_draw,
		nemomotz_object_draw_line,
		nemomotz_object_draw_rect,
		nemomotz_object_draw_round_rect,
		nemomotz_object_draw_circle,
		nemomotz_object_draw_arc
	};

	struct motzobject *object = NEMOMOTZ_OBJECT(one);

	if (nemomotz_one_has_flags(one, NEMOMOTZ_OBJECT_TRANSFORM_FLAG) != 0) {
		struct nemotoyz *toyz = motz->toyz;

		nemotoyz_save(toyz);
		nemotoyz_concat(toyz, object->matrix);

		draws[object->shape](motz, one);

		nemotoyz_restore(toyz);
	} else {
		draws[object->shape](motz, one);
	}
}

static void nemomotz_object_down(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

static void nemomotz_object_motion(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

static void nemomotz_object_up(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

static void nemomotz_object_update(struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_ONE_FLAGS_DIRTY) != 0) {
		if (nemomotz_one_has_flags_all(one, NEMOMOTZ_OBJECT_FILL_FLAG | NEMOMOTZ_OBJECT_STROKE_FLAG) != 0)
			nemotoyz_style_set_type(object->style, NEMOTOYZ_STYLE_STROKE_AND_FILL_TYPE);
		else if (nemomotz_one_has_flags(one, NEMOMOTZ_OBJECT_FILL_FLAG) != 0)
			nemotoyz_style_set_type(object->style, NEMOTOYZ_STYLE_FILL_TYPE);
		else if (nemomotz_one_has_flags(one, NEMOMOTZ_OBJECT_STROKE_FLAG) != 0)
			nemotoyz_style_set_type(object->style, NEMOTOYZ_STYLE_STROKE_TYPE);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_OBJECT_TRANSFORM_DIRTY) != 0) {
		nemotoyz_matrix_identity(object->matrix);
		nemotoyz_matrix_post_rotate(object->matrix, object->rz);
		nemotoyz_matrix_post_scale(object->matrix, object->sx, object->sy);
		nemotoyz_matrix_post_translate(object->matrix, object->tx, object->ty);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_OBJECT_COLOR_DIRTY) != 0)
		nemotoyz_style_set_color(object->style, object->r, object->g, object->b, object->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_OBJECT_STROKE_WIDTH_DIRTY) != 0)
		nemotoyz_style_set_stroke_width(object->style, object->stroke_width);
}

static void nemomotz_object_destroy(struct motzone *one)
{
	struct motzobject *object = NEMOMOTZ_OBJECT(one);

	nemomotz_one_finish(one);

	nemotoyz_style_destroy(object->style);
	nemotoyz_matrix_destroy(object->matrix);

	free(object);
}

struct motzone *nemomotz_object_create(void)
{
	struct motzobject *object;
	struct motzone *one;

	object = (struct motzobject *)malloc(sizeof(struct motzobject));
	if (object == NULL)
		return NULL;
	memset(object, 0, sizeof(struct motzobject));

	one = &object->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_object_draw;
	one->down = nemomotz_object_down;
	one->motion = nemomotz_object_motion;
	one->up = nemomotz_object_up;
	one->update = nemomotz_object_update;
	one->destroy = nemomotz_object_destroy;

	object->style = nemotoyz_style_create();
	object->matrix = nemotoyz_matrix_create();

	object->sx = 1.0f;
	object->sy = 1.0f;

	return one;
}
