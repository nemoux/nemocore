#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzpath.h>
#include <nemomisc.h>

static void nemomotz_path_draw_simple(struct nemomotz *motz, struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);
	struct nemotozz *tozz = motz->tozz;

	nemotozz_draw_path(tozz, path->style, path->path);
}

static void nemomotz_path_draw_transform(struct nemomotz *motz, struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);
	struct nemotozz *tozz = motz->tozz;

	nemotozz_save(tozz);
	nemotozz_concat(tozz, path->matrix);
	nemotozz_draw_path(tozz, path->style, path->path);
	nemotozz_restore(tozz);
}

static void nemomotz_path_draw_range(struct nemomotz *motz, struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);
	struct nemotozz *tozz = motz->tozz;

	nemotozz_draw_path(tozz, path->style, path->subpath);
}

static void nemomotz_path_draw_range_transform(struct nemomotz *motz, struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);
	struct nemotozz *tozz = motz->tozz;

	nemotozz_save(tozz);
	nemotozz_concat(tozz, path->matrix);
	nemotozz_draw_path(tozz, path->style, path->subpath);
	nemotozz_restore(tozz);
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

	nemotozz_matrix_map_point(path->inverse, &x, &y);

	if (path->x <= x && x < path->x + path->w && path->y <= y && y < path->y + path->h)
		return one;

	return NULL;
}

static void nemomotz_path_update(struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_ONE_FLAGS_DIRTY) != 0) {
		if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_RANGE_FLAG) != 0) {
			if (path->subpath == NULL)
				path->subpath = nemotozz_path_create();

			if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_TRANSFORM_FLAG) != 0)
				one->draw = nemomotz_path_draw_range_transform;
			else
				one->draw = nemomotz_path_draw_range;
		} else {
			if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_TRANSFORM_FLAG) != 0)
				one->draw = nemomotz_path_draw_transform;
			else
				one->draw = nemomotz_path_draw_simple;
		}

		if (nemomotz_one_has_flags_all(one, NEMOMOTZ_PATH_FILL_FLAG | NEMOMOTZ_PATH_STROKE_FLAG) != 0)
			nemotozz_style_set_type(path->style, NEMOTOZZ_STYLE_STROKE_AND_FILL_TYPE);
		else if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_FILL_FLAG) != 0)
			nemotozz_style_set_type(path->style, NEMOTOZZ_STYLE_FILL_TYPE);
		else if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_STROKE_FLAG) != 0)
			nemotozz_style_set_type(path->style, NEMOTOZZ_STYLE_STROKE_TYPE);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_TRANSFORM_DIRTY) != 0) {
		nemotozz_matrix_identity(path->matrix);
		nemotozz_matrix_post_rotate(path->matrix, path->rz);
		nemotozz_matrix_post_scale(path->matrix, path->sx, path->sy);
		nemotozz_matrix_post_translate(path->matrix, path->tx, path->ty);

		nemotozz_matrix_invert(path->inverse, path->matrix);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY) != 0) {
		nemotozz_path_bounds(path->path, &path->x, &path->y, &path->w, &path->h);
		nemotozz_path_measure(path->path);
	}

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_COLOR_DIRTY) != 0)
		nemotozz_style_set_color(path->style, path->r, path->g, path->b, path->a);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY) != 0)
		nemotozz_style_set_stroke_width(path->style, path->stroke_width);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_FONT_DIRTY) != 0)
		nemotozz_style_load_font(path->style, path->font_path, path->font_index);
	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_FONT_SIZE_DIRTY) != 0)
		nemotozz_style_set_font_size(path->style, path->font_size);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_PATH_RANGE_DIRTY) != 0) {
		if (nemomotz_one_has_flags(one, NEMOMOTZ_PATH_RANGE_FLAG) != 0) {
			nemotozz_path_clear(path->subpath);
			nemotozz_path_segment(path->path, path->from, path->to, path->subpath);
		}
	}
}

static void nemomotz_path_destroy(struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemomotz_one_finish(one);

	if (path->subpath != NULL)
		nemotozz_path_destroy(path->subpath);

	nemotozz_path_destroy(path->path);

	nemotozz_style_destroy(path->style);
	nemotozz_matrix_destroy(path->matrix);
	nemotozz_matrix_destroy(path->inverse);

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

	one->draw = nemomotz_path_draw_simple;
	one->down = nemomotz_path_down;
	one->motion = nemomotz_path_motion;
	one->up = nemomotz_path_up;
	one->contain = nemomotz_path_contain;
	one->update = nemomotz_path_update;
	one->destroy = nemomotz_path_destroy;

	path->style = nemotozz_style_create();
	path->matrix = nemotozz_matrix_create();
	path->inverse = nemotozz_matrix_create();

	path->path = nemotozz_path_create();

	path->sx = 1.0f;
	path->sy = 1.0f;

	return one;
}

void nemomotz_path_clear(struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_clear(path->path);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_moveto(struct motzone *one, float x, float y)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_moveto(path->path, x, y);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_lineto(struct motzone *one, float x, float y)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_lineto(path->path, x, y);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_cubicto(struct motzone *one, float x0, float y0, float x1, float y1, float x2, float y2)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_cubicto(path->path, x0, y0, x1, y1, x2, y2);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_close(struct motzone *one)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_close(path->path);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_text(struct motzone *one, float x, float y, const char *text)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_text(path->path, path->style, x, y, text);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_cmd(struct motzone *one, const char *cmd)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_cmd(path->path, cmd);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}

void nemomotz_path_svg(struct motzone *one, const char *url, float x, float y, float w, float h)
{
	struct motzpath *path = NEMOMOTZ_PATH(one);

	nemotozz_path_svg(path->path, url, x, y, w, h);

	nemomotz_one_set_dirty(one, NEMOMOTZ_PATH_SHAPE_DIRTY);
}
