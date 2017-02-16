#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzpath.h>
#include <nemomisc.h>

static void nemomotz_path_draw(struct nemomotz *motz, struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);
	struct nemotoyz *toyz = motz->toyz;

	if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_TRANSFORM_FLAG) != 0) {
		nemotoyz_save(toyz);
		nemotoyz_concat(toyz, path->matrix);
		nemotoyz_draw_path(toyz, path->style, path->path);
		nemotoyz_restore(toyz);
	} else {
		nemotoyz_draw_path(toyz, path->style, path->path);
	}
}

static void nemomotz_path_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_path_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_path_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static struct motzone *nemomotz_path_contain(struct motzone *one, float x, float y)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotoyz_matrix_map_point(path->inverse, &x, &y);

	if (path->x <= x && x < path->x + path->w && path->y <= y && y < path->y + path->h)
		return one;

	return NULL;
}

static void nemomotz_path_update(struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_ONE_FLAGS_DIRTY) != 0) {
		if (nemomotz_one_has_flags_all(one, NEMOMOTZ_PATH_FILL_FLAG | NEMOMOTZ_PATH_STROKE_FLAG) != 0)
			nemotoyz_style_set_type(path->style, NEMOTOYZ_STYLE_STROKE_AND_FILL_TYPE);
		else if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_FILL_FLAG) != 0)
			nemotoyz_style_set_type(path->style, NEMOTOYZ_STYLE_FILL_TYPE);
		else if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_STROKE_FLAG) != 0)
			nemotoyz_style_set_type(path->style, NEMOTOYZ_STYLE_STROKE_TYPE);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_TRANSFORM_DIRTY) != 0) {
		nemotoyz_matrix_identity(path->matrix);
		nemotoyz_matrix_post_rotate(path->matrix, path->rz);
		nemotoyz_matrix_post_scale(path->matrix, path->sx, path->sy);
		nemotoyz_matrix_post_translate(path->matrix, path->tx, path->ty);

		nemotoyz_matrix_invert(path->inverse, path->matrix);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY) != 0)
		nemotoyz_path_bounds(path->path, &path->x, &path->y, &path->w, &path->h);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_COLOR_DIRTY) != 0)
		nemotoyz_style_set_color(path->style, path->r, path->g, path->b, path->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY) != 0)
		nemotoyz_style_set_stroke_width(path->style, path->stroke_width);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_FONT_DIRTY) != 0)
		nemotoyz_style_load_font(path->style, path->font_path, path->font_index);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_FONT_SIZE_DIRTY) != 0)
		nemotoyz_style_set_font_size(path->style, path->font_size);
}

static void nemomotz_path_destroy(struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemomotz_one_finish(one);

	nemotoyz_path_destroy(path->path);

	nemotoyz_style_destroy(path->style);
	nemotoyz_matrix_destroy(path->matrix);
	nemotoyz_matrix_destroy(path->inverse);

	free(path);
}

struct motzone *nemomotz_path_create(void)
{
	struct motzpath *path;
	struct motzone *one;

	path = (struct motzpath *)malloc(sizeof(struct motzpath));
	if (path == NULL)
		return NULL;
	memset(path, 0, sizeof(struct motzpath));

	one = &path->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_path_draw;
	one->down = nemomotz_path_down;
	one->motion = nemomotz_path_motion;
	one->up = nemomotz_path_up;
	one->contain = nemomotz_path_contain;
	one->update = nemomotz_path_update;
	one->destroy = nemomotz_path_destroy;

	path->style = nemotoyz_style_create();
	path->matrix = nemotoyz_matrix_create();
	path->inverse = nemotoyz_matrix_create();

	path->path = nemotoyz_path_create();

	path->sx = 1.0f;
	path->sy = 1.0f;

	return one;
}
