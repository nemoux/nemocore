#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <ctype.h>
#include <math.h>

#include <nemotile.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <tilehelper.hpp>
#include <nemofs.h>
#include <nemolog.h>
#include <nemomisc.h>
#include <colorhelper.h>
#include <glhelper.h>
#include <polyhelper.h>

static void nemotile_dispatch_video_update(struct nemoplay *play, void *data);
static void nemotile_dispatch_video_done(struct nemoplay *play, void *data);

static inline float nemotile_get_column_x(int columns, int idx)
{
	float dx = columns % 2 == 0 ? 0.5f : 0.0f;

	return 2.0f / (float)columns * (idx - columns / 2 + dx);
}

static inline float nemotile_get_row_y(int rows, int idx)
{
	float dy = rows % 2 == 0 ? 0.5f : 0.0f;

	return 2.0f / (float)rows * (idx - rows / 2 + dy);
}

static inline float nemotile_get_column_sx(int columns)
{
	return 1.0f / (float)columns;
}

static inline float nemotile_get_row_sy(int rows)
{
	return 1.0f / (float)rows;
}

static inline float nemotile_get_column_tx(int columns, int idx)
{
	return 1.0f / (float)columns * idx;
}

static inline float nemotile_get_row_ty(int rows, int idx)
{
	return 1.0f / (float)rows * idx;
}

static inline float nemotile_get_tx_from_vx(int columns, int flip, float sx, float x)
{
	float tx;

	tx = (x + 1.0f) / 2.0f - sx / 2.0f;

	if (flip != 0)
		tx = 0.5f - (tx - 0.5f) - sx;

	return tx;
}

static inline float nemotile_get_ty_from_vy(int rows, int flip, float sy, float y)
{
	float ty;

	ty = (y + 1.0f) / 2.0f - sy / 2.0f;

	if (flip != 0)
		ty = 0.5f - (ty - 0.5f) - sy;

	return ty;
}

static inline float nemotile_get_camera_degree(float v, float z)
{
	float r = M_PI - atan2(v, -z);

	if (r > M_PI)
		return r - M_PI * 2.0f;

	return r;
}

static struct tileone *nemotile_one_create(int vertices)
{
	struct tileone *one;

	one = (struct tileone *)malloc(sizeof(struct tileone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct tileone));

	one->vertices = (float *)malloc(sizeof(float[3]) * vertices);
	one->texcoords = (float *)malloc(sizeof(float[2]) * vertices);
	one->diffuses = (float *)malloc(sizeof(float[4]) * vertices);
	one->normals = (float *)malloc(sizeof(float[3]) * vertices);

	one->count = vertices;
	one->type = GL_TRIANGLE_STRIP;

	one->gtransform0.tx = 0.0f;
	one->gtransform0.ty = 0.0f;
	one->gtransform0.tz = 0.0f;
	one->gtransform0.rx = 0.0f;
	one->gtransform0.ry = 0.0f;
	one->gtransform0.rz = 0.0f;
	one->gtransform0.sx = 1.0f;
	one->gtransform0.sy = 1.0f;
	one->gtransform0.sz = 1.0f;

	one->vtransform0.tx = 0.0f;
	one->vtransform0.ty = 0.0f;
	one->vtransform0.tz = 0.0f;
	one->vtransform0.rx = 0.0f;
	one->vtransform0.ry = 0.0f;
	one->vtransform0.rz = 0.0f;
	one->vtransform0.sx = 1.0f;
	one->vtransform0.sy = 1.0f;
	one->vtransform0.sz = 1.0f;

	one->ttransform0.tx = 0.0f;
	one->ttransform0.ty = 0.0f;
	one->ttransform0.r = 0.0f;
	one->ttransform0.sx = 1.0f;
	one->ttransform0.sy = 1.0f;

	one->gtransform.tx = 0.0f;
	one->gtransform.ty = 0.0f;
	one->gtransform.tz = 0.0f;
	one->gtransform.rx = 0.0f;
	one->gtransform.ry = 0.0f;
	one->gtransform.rz = 0.0f;
	one->gtransform.sx = 1.0f;
	one->gtransform.sy = 1.0f;
	one->gtransform.sz = 1.0f;

	one->vtransform.tx = 0.0f;
	one->vtransform.ty = 0.0f;
	one->vtransform.tz = 0.0f;
	one->vtransform.rx = 0.0f;
	one->vtransform.ry = 0.0f;
	one->vtransform.rz = 0.0f;
	one->vtransform.sx = 1.0f;
	one->vtransform.sy = 1.0f;
	one->vtransform.sz = 1.0f;

	one->ttransform.tx = 0.0f;
	one->ttransform.ty = 0.0f;
	one->ttransform.r = 0.0f;
	one->ttransform.sx = 1.0f;
	one->ttransform.sy = 1.0f;

	one->color[0] = 1.0f;
	one->color[1] = 1.0f;
	one->color[2] = 1.0f;
	one->color[3] = 1.0f;

	nemolist_init(&one->link);

	return one;
}

static void nemotile_one_destroy(struct tileone *one)
{
	nemolist_remove(&one->link);

	free(one->vertices);
	free(one->texcoords);
	free(one->diffuses);
	free(one->normals);

	free(one);
}

static void nemotile_one_set_index(struct tileone *one, int index)
{
	one->index = index;
}

static void nemotile_one_set_type(struct tileone *one, int type)
{
	one->type = type;
}

static void nemotile_one_group_translate(struct tileone *one, float tx, float ty, float tz)
{
	one->gtransform.tx = tx;
	one->gtransform.ty = ty;
	one->gtransform.tz = tz;
}

static void nemotile_one_group_rotate(struct tileone *one, float rx, float ry, float rz)
{
	one->gtransform.rx = rx;
	one->gtransform.ry = ry;
	one->gtransform.rz = rz;
}

static void nemotile_one_group_scale(struct tileone *one, float sx, float sy, float sz)
{
	one->gtransform.sx = sx;
	one->gtransform.sy = sy;
	one->gtransform.sz = sz;
}

static void nemotile_one_group_translate_to(struct tileone *one, float tx, float ty, float tz)
{
	one->gtransform0.tx = tx;
	one->gtransform0.ty = ty;
	one->gtransform0.tz = tz;
}

static void nemotile_one_group_rotate_to(struct tileone *one, float rx, float ry, float rz)
{
	one->gtransform0.rx = rx;
	one->gtransform0.ry = ry;
	one->gtransform0.rz = rz;
}

static void nemotile_one_group_scale_to(struct tileone *one, float sx, float sy, float sz)
{
	one->gtransform0.sx = sx;
	one->gtransform0.sy = sy;
	one->gtransform0.sz = sz;
}

static void nemotile_one_set_vertex(struct tileone *one, int index, float x, float y, float z)
{
	one->vertices[index * 3 + 0] = x;
	one->vertices[index * 3 + 1] = y;
	one->vertices[index * 3 + 2] = z;
}

static void nemotile_one_vertices_translate(struct tileone *one, float tx, float ty, float tz)
{
	one->vtransform.tx = tx;
	one->vtransform.ty = ty;
	one->vtransform.tz = tz;
}

static void nemotile_one_vertices_rotate(struct tileone *one, float rx, float ry, float rz)
{
	one->vtransform.rx = rx;
	one->vtransform.ry = ry;
	one->vtransform.rz = rz;
}

static void nemotile_one_vertices_scale(struct tileone *one, float sx, float sy, float sz)
{
	one->vtransform.sx = sx;
	one->vtransform.sy = sy;
	one->vtransform.sz = sz;
}

static void nemotile_one_vertices_translate_to(struct tileone *one, float tx, float ty, float tz)
{
	one->vtransform0.tx = tx;
	one->vtransform0.ty = ty;
	one->vtransform0.tz = tz;
}

static void nemotile_one_vertices_rotate_to(struct tileone *one, float rx, float ry, float rz)
{
	one->vtransform0.rx = rx;
	one->vtransform0.ry = ry;
	one->vtransform0.rz = rz;
}

static void nemotile_one_vertices_scale_to(struct tileone *one, float sx, float sy, float sz)
{
	one->vtransform0.sx = sx;
	one->vtransform0.sy = sy;
	one->vtransform0.sz = sz;
}

static void nemotile_one_set_texcoord(struct tileone *one, int index, float tx, float ty)
{
	one->texcoords[index * 2 + 0] = tx;
	one->texcoords[index * 2 + 1] = ty;
}

static void nemotile_one_texcoords_translate(struct tileone *one, float tx, float ty)
{
	one->ttransform.tx = tx;
	one->ttransform.ty = ty;
}

static void nemotile_one_texcoords_rotate(struct tileone *one, float r)
{
	one->ttransform.r = r;
}

static void nemotile_one_texcoords_scale(struct tileone *one, float sx, float sy)
{
	one->ttransform.sx = sx;
	one->ttransform.sy = sy;
}

static void nemotile_one_texcoords_translate_to(struct tileone *one, float tx, float ty)
{
	one->ttransform0.tx = tx;
	one->ttransform0.ty = ty;
}

static void nemotile_one_texcoords_rotate_to(struct tileone *one, float r)
{
	one->ttransform0.r = r;
}

static void nemotile_one_texcoords_scale_to(struct tileone *one, float sx, float sy)
{
	one->ttransform0.sx = sx;
	one->ttransform0.sy = sy;
}

static void nemotile_one_set_texture(struct tileone *one, struct showone *canvas)
{
	one->texture = canvas;
}

static void nemotile_one_set_normal(struct tileone *one, int index, float x, float y, float z)
{
	one->normals[index * 3 + 0] = x;
	one->normals[index * 3 + 1] = y;
	one->normals[index * 3 + 2] = z;
}

static void nemotile_one_set_color(struct tileone *one, float r, float g, float b, float a)
{
	one->color[0] = r;
	one->color[1] = g;
	one->color[2] = b;
	one->color[3] = a;
}

static struct tileone *nemotile_pick_simple(struct nemotile *tile, float x, float y)
{
	struct tileone *one;
	struct nemomatrix matrix, inverse;

	x = (x / tile->width * 2.0f) - 1.0f;
	y = (y / tile->height * 2.0f) - 1.0f;

	nemolist_for_each_reverse(one, &tile->tile_list, link) {
		nemomatrix_init_identity(&matrix);
		nemomatrix_scale_xyz(&matrix, one->vtransform.sx, one->vtransform.sy, one->vtransform.sz);
		nemomatrix_rotate_x(&matrix, cos(one->vtransform.rx), sin(one->vtransform.rx));
		nemomatrix_rotate_y(&matrix, cos(one->vtransform.ry), sin(one->vtransform.ry));
		nemomatrix_rotate_z(&matrix, cos(one->vtransform.rz), sin(one->vtransform.rz));
		nemomatrix_translate_xyz(&matrix, one->vtransform.tx, one->vtransform.ty, one->vtransform.tz);

		if (nemomatrix_invert(&inverse, &matrix) == 0) {
			float tx = x;
			float ty = y;

			nemomatrix_transform(&inverse, &tx, &ty);

			if (-1.0f < tx && tx < 1.0f && -1.0f < ty && ty < 1.0f)
				return one;
		}
	}

	return NULL;
}

static struct tileone *nemotile_pick_complex(struct nemotile *tile, float x, float y, float *t, float *u, float *v)
{
	struct tileone *one;
	struct nemomatrix projection;
	struct nemomatrix modelview;
	float beta, gamma;
	int i, j;

	if (tile->is_dynamic_perspective == 0) {
		nemomatrix_init_identity(&projection);
		nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
		nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
		nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
		nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
		nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);
		nemomatrix_perspective(&projection,
				tile->perspective.left,
				tile->perspective.right,
				tile->perspective.bottom,
				tile->perspective.top,
				tile->perspective.near,
				tile->perspective.far);
	} else {
		nemomatrix_init_identity(&projection);
		nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
		nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
		nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
		nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
		nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);
		nemomatrix_asymmetric(&projection, tile->asymmetric.a, tile->asymmetric.b, tile->asymmetric.c, tile->asymmetric.e, tile->asymmetric.near, tile->asymmetric.far);
	}

	for (i = tile->ntiles - 1; i >= 0; i--) {
		one = tile->tiles[i];

		nemomatrix_init_identity(&modelview);
		nemomatrix_scale_xyz(&modelview, one->vtransform.sx, one->vtransform.sy, one->vtransform.sz);
		nemomatrix_rotate_x(&modelview, cos(one->vtransform.rx), sin(one->vtransform.rx));
		nemomatrix_rotate_y(&modelview, cos(one->vtransform.ry), sin(one->vtransform.ry));
		nemomatrix_rotate_z(&modelview, cos(one->vtransform.rz), sin(one->vtransform.rz));
		nemomatrix_translate_xyz(&modelview, one->vtransform.tx, one->vtransform.ty, one->vtransform.tz);
		nemomatrix_scale_xyz(&modelview, one->gtransform.sx, one->gtransform.sy, one->gtransform.sz);
		nemomatrix_rotate_x(&modelview, cos(one->gtransform.rx), sin(one->gtransform.rx));
		nemomatrix_rotate_y(&modelview, cos(one->gtransform.ry), sin(one->gtransform.ry));
		nemomatrix_rotate_z(&modelview, cos(one->gtransform.rz), sin(one->gtransform.rz));
		nemomatrix_translate_xyz(&modelview, one->gtransform.tx, one->gtransform.ty, one->gtransform.tz);

		for (j = 0; j < one->count - 2; j++) {
			if (poly_pick_triangle(
						&projection,
						tile->width, tile->height,
						&modelview,
						&one->vertices[j * 3 + 0],
						&one->vertices[j * 3 + 3],
						&one->vertices[j * 3 + 6],
						x, y,
						t, &beta, &gamma) > 0) {
				*u = 1.0f - (one->texcoords[j * 2 + 2] * beta + one->texcoords[j * 2 + 4] * gamma + one->texcoords[j * 2 + 0] * (1.0f - beta - gamma));
				*v = 1.0f - (one->texcoords[j * 2 + 3] * beta + one->texcoords[j * 2 + 5] * gamma + one->texcoords[j * 2 + 1] * (1.0f - beta - gamma));

				return one;
			}
		}
	}

	nemolist_for_each(one, &tile->wall_list, link) {
		nemomatrix_init_identity(&modelview);
		nemomatrix_scale_xyz(&modelview, one->vtransform.sx, one->vtransform.sy, one->vtransform.sz);
		nemomatrix_rotate_x(&modelview, cos(one->vtransform.rx), sin(one->vtransform.rx));
		nemomatrix_rotate_y(&modelview, cos(one->vtransform.ry), sin(one->vtransform.ry));
		nemomatrix_rotate_z(&modelview, cos(one->vtransform.rz), sin(one->vtransform.rz));
		nemomatrix_translate_xyz(&modelview, one->vtransform.tx, one->vtransform.ty, one->vtransform.tz);
		nemomatrix_scale_xyz(&modelview, one->gtransform.sx, one->gtransform.sy, one->gtransform.sz);
		nemomatrix_rotate_x(&modelview, cos(one->gtransform.rx), sin(one->gtransform.rx));
		nemomatrix_rotate_y(&modelview, cos(one->gtransform.ry), sin(one->gtransform.ry));
		nemomatrix_rotate_z(&modelview, cos(one->gtransform.rz), sin(one->gtransform.rz));
		nemomatrix_translate_xyz(&modelview, one->gtransform.tx, one->gtransform.ty, one->gtransform.tz);

		for (j = 0; j < one->count - 2; j++) {
			if (poly_pick_triangle(
						&projection,
						tile->width, tile->height,
						&modelview,
						&one->vertices[j * 3 + 0],
						&one->vertices[j * 3 + 3],
						&one->vertices[j * 3 + 6],
						x, y,
						t, &beta, &gamma) > 0) {
				*u = 1.0f - (one->texcoords[j * 2 + 2] * beta + one->texcoords[j * 2 + 4] * gamma + one->texcoords[j * 2 + 0] * (1.0f - beta - gamma));
				*v = 1.0f - (one->texcoords[j * 2 + 3] * beta + one->texcoords[j * 2 + 5] * gamma + one->texcoords[j * 2 + 1] * (1.0f - beta - gamma));

				return one;
			}
		}
	}

	return NULL;
}

static struct tileone *nemotile_find_one(struct nemotile *tile, int index)
{
	struct tileone *one;

	nemolist_for_each(one, &tile->tile_list, link) {
		if (one->index == index)
			return one;
	}

	return NULL;
}

static void nemotile_prepare_depth(struct nemotile *tile)
{
	struct tileone *one;
	int index = 0;

	tile->ntiles = nemolist_length(&tile->tile_list);
	tile->tiles = (struct tileone **)malloc(sizeof(struct tileone *) * tile->ntiles);

	nemolist_for_each(one, &tile->tile_list, link) {
		tile->tiles[index++] = one;
	}
}

static void nemotile_finish_depth(struct nemotile *tile)
{
	free(tile->tiles);

	tile->ntiles = 0;
}

static int nemotile_compare_depth(const void *a, const void *b)
{
	const struct tileone *one0 = *((const struct tileone **)a);
	const struct tileone *one1 = *((const struct tileone **)b);

	if (one0->vtransform.tz < one1->vtransform.tz)
		return -1;
	if (one0->vtransform.tz == one1->vtransform.tz)
		return 0;

	return 1;
}

static void nemotile_sort_depth(struct nemotile *tile)
{
	qsort(tile->tiles, tile->ntiles, sizeof(struct tileone *), nemotile_compare_depth);
}

static void nemotile_render_2d_one(struct nemotile *tile, struct nemomatrix *projection, struct tileone *one)
{
	struct nemomatrix vtransform;
	struct nemomatrix ttransform;

	nemomatrix_init_identity(&vtransform);
	nemomatrix_scale_xyz(&vtransform, one->vtransform.sx, one->vtransform.sy, one->vtransform.sz);
	nemomatrix_rotate_x(&vtransform, cos(one->vtransform.rx), sin(one->vtransform.rx));
	nemomatrix_rotate_y(&vtransform, cos(one->vtransform.ry), sin(one->vtransform.ry));
	nemomatrix_rotate_z(&vtransform, cos(one->vtransform.rz), sin(one->vtransform.rz));
	nemomatrix_translate_xyz(&vtransform, one->vtransform.tx, one->vtransform.ty, one->vtransform.tz);

	nemomatrix_init_identity(&ttransform);
	nemomatrix_scale(&ttransform, one->ttransform.sx, one->ttransform.sy);
	nemomatrix_rotate(&ttransform, cos(one->ttransform.r), sin(one->ttransform.r));
	nemomatrix_translate(&ttransform, one->ttransform.tx, one->ttransform.ty);

	glUseProgram(tile->programs[0]);
	glBindAttribLocation(tile->programs[0], 0, "position");
	glBindAttribLocation(tile->programs[0], 1, "texcoord");

	glUniform1i(tile->utexture0, 0);
	glUniformMatrix4fv(tile->uprojection0, 1, GL_FALSE, projection->d);
	glUniformMatrix4fv(tile->uvtransform0, 1, GL_FALSE, vtransform.d);
	glUniformMatrix4fv(tile->uttransform0, 1, GL_FALSE, ttransform.d);
	glUniform4fv(tile->ucolor0, 1, one->color);

	glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_effective_texture(one->texture));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->texcoords[0]);
	glEnableVertexAttribArray(1);

	glDrawArrays(one->type, 0, one->count);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemotile_render_3d_one(struct nemotile *tile, struct nemomatrix *projection, struct tileone *one)
{
	struct nemomatrix vtransform;
	struct nemomatrix ttransform;

	nemomatrix_init_identity(&vtransform);
	nemomatrix_scale_xyz(&vtransform, one->vtransform.sx, one->vtransform.sy, one->vtransform.sz);
	nemomatrix_rotate_x(&vtransform, cos(one->vtransform.rx), sin(one->vtransform.rx));
	nemomatrix_rotate_y(&vtransform, cos(one->vtransform.ry), sin(one->vtransform.ry));
	nemomatrix_rotate_z(&vtransform, cos(one->vtransform.rz), sin(one->vtransform.rz));
	nemomatrix_translate_xyz(&vtransform, one->vtransform.tx, one->vtransform.ty, one->vtransform.tz);
	nemomatrix_scale_xyz(&vtransform, one->gtransform.sx, one->gtransform.sy, one->gtransform.sz);
	nemomatrix_rotate_x(&vtransform, cos(one->gtransform.rx), sin(one->gtransform.rx));
	nemomatrix_rotate_y(&vtransform, cos(one->gtransform.ry), sin(one->gtransform.ry));
	nemomatrix_rotate_z(&vtransform, cos(one->gtransform.rz), sin(one->gtransform.rz));
	nemomatrix_translate_xyz(&vtransform, one->gtransform.tx, one->gtransform.ty, one->gtransform.tz);

	nemomatrix_init_identity(&ttransform);
	nemomatrix_scale(&ttransform, one->ttransform.sx, one->ttransform.sy);
	nemomatrix_rotate(&ttransform, cos(one->ttransform.r), sin(one->ttransform.r));
	nemomatrix_translate(&ttransform, one->ttransform.tx, one->ttransform.ty);

	if (one->has_diffuses == 0) {
		glUseProgram(tile->programs[0]);
		glBindAttribLocation(tile->programs[0], 0, "position");
		glBindAttribLocation(tile->programs[0], 1, "texcoord");

		glUniform1i(tile->utexture0, 0);
		glUniformMatrix4fv(tile->uprojection0, 1, GL_FALSE, projection->d);
		glUniformMatrix4fv(tile->uvtransform0, 1, GL_FALSE, vtransform.d);
		glUniformMatrix4fv(tile->uttransform0, 1, GL_FALSE, ttransform.d);
		glUniform4fv(tile->ucolor0, 1, one->color);

		glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_effective_texture(one->texture));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->texcoords[0]);
		glEnableVertexAttribArray(1);
	} else {
		glUseProgram(tile->programs[2]);
		glBindAttribLocation(tile->programs[2], 0, "position");
		glBindAttribLocation(tile->programs[2], 1, "diffuse");

		glUniformMatrix4fv(tile->uprojection2, 1, GL_FALSE, projection->d);
		glUniformMatrix4fv(tile->uvtransform2, 1, GL_FALSE, vtransform.d);
		glUniform4fv(tile->ucolor2, 1, one->color);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &one->diffuses[0]);
		glEnableVertexAttribArray(1);
	}

	glDrawArrays(one->type, 0, one->count);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemotile_render_3d_lighting_one(struct nemotile *tile, struct nemomatrix *projection, struct tileone *one)
{
	struct nemomatrix vtransform;
	struct nemomatrix ttransform;

	nemomatrix_init_identity(&vtransform);
	nemomatrix_scale_xyz(&vtransform, one->vtransform.sx, one->vtransform.sy, one->vtransform.sz);
	nemomatrix_rotate_x(&vtransform, cos(one->vtransform.rx), sin(one->vtransform.rx));
	nemomatrix_rotate_y(&vtransform, cos(one->vtransform.ry), sin(one->vtransform.ry));
	nemomatrix_rotate_z(&vtransform, cos(one->vtransform.rz), sin(one->vtransform.rz));
	nemomatrix_translate_xyz(&vtransform, one->vtransform.tx, one->vtransform.ty, one->vtransform.tz);
	nemomatrix_scale_xyz(&vtransform, one->gtransform.sx, one->gtransform.sy, one->gtransform.sz);
	nemomatrix_rotate_x(&vtransform, cos(one->gtransform.rx), sin(one->gtransform.rx));
	nemomatrix_rotate_y(&vtransform, cos(one->gtransform.ry), sin(one->gtransform.ry));
	nemomatrix_rotate_z(&vtransform, cos(one->gtransform.rz), sin(one->gtransform.rz));
	nemomatrix_translate_xyz(&vtransform, one->gtransform.tx, one->gtransform.ty, one->gtransform.tz);

	nemomatrix_init_identity(&ttransform);
	nemomatrix_scale(&ttransform, one->ttransform.sx, one->ttransform.sy);
	nemomatrix_rotate(&ttransform, cos(one->ttransform.r), sin(one->ttransform.r));
	nemomatrix_translate(&ttransform, one->ttransform.tx, one->ttransform.ty);

	if (one->has_diffuses == 0) {
		glUseProgram(tile->programs[1]);
		glBindAttribLocation(tile->programs[1], 0, "position");
		glBindAttribLocation(tile->programs[1], 1, "texcoord");
		glBindAttribLocation(tile->programs[1], 2, "normal");

		glUniform1i(tile->utexture1, 0);
		glUniformMatrix4fv(tile->uprojection1, 1, GL_FALSE, projection->d);
		glUniformMatrix4fv(tile->uvtransform1, 1, GL_FALSE, vtransform.d);
		glUniformMatrix4fv(tile->uttransform1, 1, GL_FALSE, ttransform.d);
		glUniform4fv(tile->uambient1, 1, tile->ambient);
		glUniform4fv(tile->ulight1, 1, tile->light);

		glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_effective_texture(one->texture));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->texcoords[0]);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->normals[0]);
		glEnableVertexAttribArray(2);
	} else {
		glUseProgram(tile->programs[3]);
		glBindAttribLocation(tile->programs[3], 0, "position");
		glBindAttribLocation(tile->programs[3], 1, "diffuse");
		glBindAttribLocation(tile->programs[3], 2, "normal");

		glUniformMatrix4fv(tile->uprojection3, 1, GL_FALSE, projection->d);
		glUniformMatrix4fv(tile->uvtransform3, 1, GL_FALSE, vtransform.d);
		glUniform4fv(tile->uambient3, 1, tile->ambient);
		glUniform4fv(tile->ulight3, 1, tile->light);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &one->diffuses[0]);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), &one->normals[0]);
		glEnableVertexAttribArray(2);
	}

	glDrawArrays(one->type, 0, one->count);

	glBindTexture(GL_TEXTURE_2D, 0);
}

static void nemotile_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);
	struct tileone *one;
	struct nemotrans *trans, *ntrans;
	struct nemomatrix projection;
	int i;

	glBindFramebuffer(GL_FRAMEBUFFER, tile->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);

	if (tile->is_3d == 0) {
		nemomatrix_init_identity(&projection);
		nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
		nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
		nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
		nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
		nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);

		nemolist_for_each(one, &tile->tile_list, link) {
			nemotile_render_2d_one(tile, &projection, one);
		}
	} else if (tile->is_lighting == 0) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		if (tile->is_dynamic_perspective == 0) {
			nemomatrix_init_identity(&projection);
			nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
			nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
			nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
			nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
			nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);
			nemomatrix_perspective(&projection,
					tile->perspective.left,
					tile->perspective.right,
					tile->perspective.bottom,
					tile->perspective.top,
					tile->perspective.near,
					tile->perspective.far);
		} else {
			nemomatrix_init_identity(&projection);
			nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
			nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
			nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
			nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
			nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);
			nemomatrix_asymmetric(&projection, tile->asymmetric.a, tile->asymmetric.b, tile->asymmetric.c, tile->asymmetric.e, tile->asymmetric.near, tile->asymmetric.far);
		}

		nemotile_sort_depth(tile);

		nemolist_for_each(one, &tile->wall_list, link) {
			nemotile_render_3d_one(tile, &projection, one);
		}

		for (i = 0; i < tile->ntiles; i++) {
			nemotile_render_3d_one(tile, &projection, tile->tiles[i]);
		}

		if (tile->cube != NULL) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			nemotile_render_3d_one(tile, &projection, tile->cube);

			glDisable(GL_CULL_FACE);
		}

		if (tile->mesh != NULL) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			nemotile_render_3d_one(tile, &projection, tile->mesh);

			glDisable(GL_CULL_FACE);
		}

		if (tile->ceil != NULL)
			nemotile_render_3d_one(tile, &projection, tile->ceil);

		nemolist_for_each(one, &tile->test_list, link) {
			nemotile_render_3d_one(tile, &projection, one);
		}
	} else {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS);

		if (tile->is_dynamic_perspective == 0) {
			nemomatrix_init_identity(&projection);
			nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
			nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
			nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
			nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
			nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);
			nemomatrix_perspective(&projection,
					tile->perspective.left,
					tile->perspective.right,
					tile->perspective.bottom,
					tile->perspective.top,
					tile->perspective.near,
					tile->perspective.far);
		} else {
			nemomatrix_init_identity(&projection);
			nemomatrix_scale_xyz(&projection, tile->projection.sx, tile->projection.sy, tile->projection.sz);
			nemomatrix_rotate_x(&projection, cos(tile->projection.rx), sin(tile->projection.rx));
			nemomatrix_rotate_y(&projection, cos(tile->projection.ry), sin(tile->projection.ry));
			nemomatrix_rotate_z(&projection, cos(tile->projection.rz), sin(tile->projection.rz));
			nemomatrix_translate_xyz(&projection, tile->projection.tx, tile->projection.ty, tile->projection.tz);
			nemomatrix_asymmetric(&projection, tile->asymmetric.a, tile->asymmetric.b, tile->asymmetric.c, tile->asymmetric.e, tile->asymmetric.near, tile->asymmetric.far);
		}

		nemotile_sort_depth(tile);

		nemolist_for_each(one, &tile->wall_list, link) {
			nemotile_render_3d_lighting_one(tile, &projection, one);
		}

		for (i = 0; i < tile->ntiles; i++) {
			nemotile_render_3d_lighting_one(tile, &projection, tile->tiles[i]);
		}

		if (tile->cube != NULL) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			nemotile_render_3d_lighting_one(tile, &projection, tile->cube);

			glDisable(GL_CULL_FACE);
		}

		if (tile->mesh != NULL) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);

			nemotile_render_3d_lighting_one(tile, &projection, tile->mesh);

			glDisable(GL_CULL_FACE);
		}

		if (tile->ceil != NULL)
			nemotile_render_3d_lighting_one(tile, &projection, tile->ceil);

		nemolist_for_each(one, &tile->test_list, link) {
			nemotile_render_3d_lighting_one(tile, &projection, one);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
}

static void nemotile_dispatch_video_trans_done(struct nemotrans *trans, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;
	struct tileone *one;

	nemoplay_back_destroy_decoder(tile->decoderback);
	nemoplay_back_destroy_audio(tile->audioback);
	nemoplay_back_destroy_video(tile->videoback);
	nemoplay_destroy(tile->play);

	tile->imovies = (tile->imovies + 1) % nemofs_dir_get_filecount(tile->movies);

	tile->play = nemoplay_create();
	nemoplay_load_media(tile->play, nemofs_dir_get_filepath(tile->movies, tile->imovies));

	nemoshow_canvas_set_size(tile->video,
			nemoplay_get_video_width(tile->play),
			nemoplay_get_video_height(tile->play));

	tile->decoderback = nemoplay_back_create_decoder(tile->play);
	tile->audioback = nemoplay_back_create_audio_by_ao(tile->play);
	tile->videoback = nemoplay_back_create_video_by_timer(tile->play, tile->tool);
	nemoplay_back_set_video_texture(tile->videoback,
			nemoshow_canvas_get_texture(tile->video),
			nemoplay_get_video_width(tile->play),
			nemoplay_get_video_height(tile->play));
	nemoplay_back_set_video_update(tile->videoback, nemotile_dispatch_video_update);
	nemoplay_back_set_video_done(tile->videoback, nemotile_dispatch_video_done);
	nemoplay_back_set_video_data(tile->videoback, tile);

	nemolist_for_each(one, &tile->tile_list, link) {
		trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
				random_get_int(800, 1600),
				random_get_int(100, 1200));

		nemotrans_set_float(trans, &one->vtransform.tz, random_get_double(-0.25f, 0.45f));

		nemotrans_group_attach_trans(tile->trans_group, trans);
	}

	nemotimer_set_timeout(tile->timer, tile->timeout);
}

static void nemotile_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);
	struct tileone *one;
	struct nemotrans *trans;
	float dx, dy;

	if (tile->is_3d == 0) {
		if (tile->is_single != 0) {
			if (nemoshow_event_is_pointer_left_down(show, event)) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 2400),
							random_get_int(100, 300));

					nemotrans_set_float(trans, &one->ttransform.tx, 0.0f);
					nemotrans_set_float(trans, &one->ttransform.ty, 0.0f);
					nemotrans_set_float(trans, &one->ttransform.sx, 1.0f);
					nemotrans_set_float(trans, &one->ttransform.sy, 1.0f);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (nemoshow_event_is_pointer_left_up(show, event)) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 2400),
							random_get_int(100, 300));

					nemotrans_set_float(trans, &one->ttransform.tx, nemotile_get_tx_from_vx(tile->columns, tile->flip, one->vtransform.sx, one->vtransform.tx));
					nemotrans_set_float(trans, &one->ttransform.ty, nemotile_get_ty_from_vy(tile->rows, tile->flip, one->vtransform.sy, one->vtransform.ty));
					nemotrans_set_float(trans, &one->ttransform.sx, one->vtransform.sx);
					nemotrans_set_float(trans, &one->ttransform.sy, one->vtransform.sy);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}

			if (nemoshow_event_is_pointer_right_down(show, event)) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1800),
							random_get_int(100, 300));

					nemotrans_set_float(trans, &one->color[0], 0.08f);
					nemotrans_set_float(trans, &one->color[1], 0.08f);
					nemotrans_set_float(trans, &one->color[2], 0.08f);
					nemotrans_set_float(trans, &one->color[3], 0.08f);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (nemoshow_event_is_pointer_right_up(show, event)) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1800),
							random_get_int(100, 300));

					nemotrans_set_float(trans, &one->color[0], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[1], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[2], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[3], random_get_double(tile->brightness, 1.0f));

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}

			if (nemoshow_event_is_touch_down(show, event)) {
				struct tileone *pone;

				pone = nemotile_pick_simple(tile, nemoshow_event_get_x(event), nemoshow_event_get_y(event));
				if (pone != NULL) {
					nemolist_remove(&pone->link);
					nemolist_insert_tail(&tile->tile_list, &pone->link);

					nemolist_for_each(one, &tile->tile_list, link) {
						trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
								random_get_int(600, 1200),
								random_get_int(30, 300));

						nemotrans_set_float(trans, &one->vtransform.tx, pone->vtransform.tx);
						nemotrans_set_float(trans, &one->vtransform.ty, pone->vtransform.ty);

						nemotrans_group_attach_trans(tile->trans_group, trans);
					}
				}
			} else if (nemoshow_event_is_touch_up(show, event)) {
				nemolist_for_each(one, &tile->tile_list, link) {
					dx = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dy = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(600, 1200),
							random_get_int(30, 300));

					nemotrans_set_float(trans, &one->vtransform.tx, one->vtransform0.tx * dx);
					nemotrans_set_float(trans, &one->vtransform.ty, one->vtransform0.ty * dy);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}
		} else {
			if (nemoshow_event_is_pointer_left_down(show, event)) {
				tile->slideshow = (tile->slideshow + 1) % 4;

				if (tile->slideshow == 0) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1600),
							random_get_int(200, 400));

					nemotrans_set_float(trans, &tile->projection.rx, 0.0f);
					nemotrans_set_float(trans, &tile->projection.ry, 0.0f);
					nemotrans_set_float(trans, &tile->projection.rz, 0.0f);

					nemotrans_set_float(trans, &tile->projection.tx, 0.0f);
					nemotrans_set_float(trans, &tile->projection.ty, 0.0f);
					nemotrans_set_float(trans, &tile->projection.tz, 0.0f);
					nemotrans_set_float(trans, &tile->projection.sx, 1.0f);
					nemotrans_set_float(trans, &tile->projection.sy, 1.0f);
					nemotrans_set_float(trans, &tile->projection.sz, 1.0f);

					nemotrans_group_attach_trans(tile->trans_group, trans);

					nemolist_for_each(one, &tile->tile_list, link) {
						trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
								random_get_int(800, 1600),
								random_get_int(200, 400));

						nemotrans_set_float(trans, &one->vtransform.rx, 0.0f);
						nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
						nemotrans_set_float(trans, &one->vtransform.rz, 0.0f);

						nemotrans_set_float(trans, &one->vtransform.tx, one->vtransform0.tx);
						nemotrans_set_float(trans, &one->vtransform.ty, one->vtransform0.ty);
						nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx);
						nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy);

						nemotrans_set_float(trans, &one->color[0], 1.0f);
						nemotrans_set_float(trans, &one->color[1], 1.0f);
						nemotrans_set_float(trans, &one->color[2], 1.0f);
						nemotrans_set_float(trans, &one->color[3], 1.0f);

						nemotrans_group_attach_trans(tile->trans_group, trans);
					}
				} else if (tile->slideshow == 1) {
					tile->csprites = tile->nsprites / 2;

					nemolist_for_each(one, &tile->tile_list, link) {
						if (one->index == tile->csprites) {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 1200, 0);

							nemotrans_set_float(trans, &one->vtransform.tx, (one->index - tile->csprites) * one->vtransform0.sx * 3.0f);
							nemotrans_set_float(trans, &one->vtransform.ty, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.rx, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.rz, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 3.6f);
							nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 3.6f);

							nemotrans_set_float(trans, &one->color[0], 1.0f);
							nemotrans_set_float(trans, &one->color[1], 1.0f);
							nemotrans_set_float(trans, &one->color[2], 1.0f);
							nemotrans_set_float(trans, &one->color[3], 1.0f);

							nemotrans_group_attach_trans(tile->trans_group, trans);

							tile->pone = one;
						} else {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
									random_get_int(800, 1600),
									random_get_int(200, 400));

							nemotrans_set_float(trans, &one->vtransform.tx, (one->index - tile->csprites) * one->vtransform0.sx * 3.0f);
							nemotrans_set_float(trans, &one->vtransform.ty, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.rx, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.ry, M_PI);
							nemotrans_set_float(trans, &one->vtransform.rz, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 1.0f);
							nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 1.0f);

							nemotrans_set_float(trans, &one->color[0], 0.12f);
							nemotrans_set_float(trans, &one->color[1], 0.12f);
							nemotrans_set_float(trans, &one->color[2], 0.12f);
							nemotrans_set_float(trans, &one->color[3], 0.12f);

							nemotrans_group_attach_trans(tile->trans_group, trans);
						}
					}

					nemolist_remove(&tile->pone->link);
					nemolist_insert_tail(&tile->tile_list, &tile->pone->link);
				} else if (tile->slideshow == 2) {
					tile->csprites = tile->nsprites / 2;

					nemolist_for_each(one, &tile->tile_list, link) {
						if (one->index == tile->csprites) {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 1200, 0);

							nemotrans_set_float(trans, &one->vtransform.tx, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.ty, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.rx, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.rz, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 3.6f);
							nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 3.6f);

							nemotrans_set_float(trans, &one->color[0], 1.0f);
							nemotrans_set_float(trans, &one->color[1], 1.0f);
							nemotrans_set_float(trans, &one->color[2], 1.0f);
							nemotrans_set_float(trans, &one->color[3], 1.0f);

							nemotrans_group_attach_trans(tile->trans_group, trans);

							tile->pone = one;
						} else {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
									random_get_int(800, 1600),
									random_get_int(200, 400));

							dx = cos(one->index * M_PI * 2.0f / tile->nsprites) * 0.75f;
							dy = sin(one->index * M_PI * 2.0f / tile->nsprites) * 0.75f;

							nemotrans_set_float(trans, &one->vtransform.tx, dx);
							nemotrans_set_float(trans, &one->vtransform.ty, dy);

							nemotrans_set_float(trans, &one->vtransform.rx, M_PI);
							nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.rz, -atan2(dx, dy));

							nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 0.75f);
							nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 0.75f);

							nemotrans_set_float(trans, &one->color[0], 0.12f);
							nemotrans_set_float(trans, &one->color[1], 0.12f);
							nemotrans_set_float(trans, &one->color[2], 0.12f);
							nemotrans_set_float(trans, &one->color[3], 0.12f);

							nemotrans_group_attach_trans(tile->trans_group, trans);
						}
					}

					nemolist_remove(&tile->pone->link);
					nemolist_insert_tail(&tile->tile_list, &tile->pone->link);
				} else if (tile->slideshow == 3) {
					tile->csprites = tile->columns / 2;
					tile->rsprites = tile->rows / 2;

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1600),
							random_get_int(200, 400));

					nemotrans_set_float(trans, &tile->projection.tx, tile->csprites * -2.0f);
					nemotrans_set_float(trans, &tile->projection.ty, tile->rsprites * -2.0f);

					nemotrans_group_attach_trans(tile->trans_group, trans);

					nemolist_for_each(one, &tile->tile_list, link) {
						trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
								random_get_int(800, 1600),
								random_get_int(200, 400));

						nemotrans_set_float(trans, &one->vtransform.rx, 0.0f);
						nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
						nemotrans_set_float(trans, &one->vtransform.rz, 0.0f);

						nemotrans_set_float(trans, &one->vtransform.tx, (one->index % tile->columns) * 2.0f);
						nemotrans_set_float(trans, &one->vtransform.ty, (one->index / tile->columns) * 2.0f);
						nemotrans_set_float(trans, &one->vtransform.sx, 1.0f);
						nemotrans_set_float(trans, &one->vtransform.sy, 1.0f);

						nemotrans_set_float(trans, &one->color[0], 1.0f);
						nemotrans_set_float(trans, &one->color[1], 1.0f);
						nemotrans_set_float(trans, &one->color[2], 1.0f);
						nemotrans_set_float(trans, &one->color[3], 1.0f);

						nemotrans_group_attach_trans(tile->trans_group, trans);
					}
				}
			}

			if (nemoshow_event_is_keyboard_down(show, event)) {
				if (tile->slideshow == 1) {
					int csprites = tile->csprites;

					if (nemoshow_event_get_value(event) == KEY_LEFT) {
						csprites = CLAMP(tile->csprites - 1, 0, tile->nsprites - 1);
					} else if (nemoshow_event_get_value(event) == KEY_RIGHT) {
						csprites = CLAMP(tile->csprites + 1, 0, tile->nsprites - 1);
					}

					if (tile->csprites != csprites) {
						tile->csprites = csprites;

						nemolist_for_each(one, &tile->tile_list, link) {
							if (one->index == tile->csprites) {
								trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 600, 0);

								nemotrans_set_float(trans, &one->vtransform.tx, (one->index - tile->csprites) * one->vtransform0.sx * 3.0f);
								nemotrans_set_float(trans, &one->vtransform.ty, 0.0f);

								nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);

								nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 3.6f);
								nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 3.6f);

								nemotrans_set_float(trans, &one->color[0], 1.0f);
								nemotrans_set_float(trans, &one->color[1], 1.0f);
								nemotrans_set_float(trans, &one->color[2], 1.0f);
								nemotrans_set_float(trans, &one->color[3], 1.0f);

								nemotrans_group_attach_trans(tile->trans_group, trans);

								tile->pone = one;
							} else {
								trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 600, 0);

								nemotrans_set_float(trans, &one->vtransform.tx, (one->index - tile->csprites) * one->vtransform0.sx * 3.0f);
								nemotrans_set_float(trans, &one->vtransform.ty, 0.0f);

								nemotrans_set_float(trans, &one->vtransform.ry, M_PI);

								nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 1.0f);
								nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 1.0f);

								nemotrans_set_float(trans, &one->color[0], 0.12f);
								nemotrans_set_float(trans, &one->color[1], 0.12f);
								nemotrans_set_float(trans, &one->color[2], 0.12f);
								nemotrans_set_float(trans, &one->color[3], 0.12f);

								nemotrans_group_attach_trans(tile->trans_group, trans);
							}
						}

						nemolist_remove(&tile->pone->link);
						nemolist_insert_tail(&tile->tile_list, &tile->pone->link);
					}
				} else if (tile->slideshow == 2) {
					int csprites = tile->csprites;

					if (nemoshow_event_get_value(event) == KEY_LEFT) {
						csprites = (tile->csprites + tile->nsprites - 1) % tile->nsprites;
					} else if (nemoshow_event_get_value(event) == KEY_RIGHT) {
						csprites = (tile->csprites + 1) % tile->nsprites;
					}

					if (tile->csprites != csprites) {
						tile->csprites = csprites;

						if (tile->pone != NULL) {
							one = tile->pone;

							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 600, 0);

							dx = cos(one->index * M_PI * 2.0f / tile->nsprites) * 0.75f;
							dy = sin(one->index * M_PI * 2.0f / tile->nsprites) * 0.75f;

							nemotrans_set_float(trans, &one->vtransform.tx, dx);
							nemotrans_set_float(trans, &one->vtransform.ty, dy);

							nemotrans_set_float(trans, &one->vtransform.rx, M_PI);
							nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.rz, -atan2(dx, dy));

							nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 0.75f);
							nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 0.75f);

							nemotrans_set_float(trans, &one->color[0], 0.12f);
							nemotrans_set_float(trans, &one->color[1], 0.12f);
							nemotrans_set_float(trans, &one->color[2], 0.12f);
							nemotrans_set_float(trans, &one->color[3], 0.12f);

							nemotrans_group_attach_trans(tile->trans_group, trans);
						}

						tile->pone = nemotile_find_one(tile, tile->csprites);
						if (tile->pone != NULL) {
							one = tile->pone;

							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 600, 0);

							nemotrans_set_float(trans, &one->vtransform.tx, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.ty, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.rx, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.ry, 0.0f);
							nemotrans_set_float(trans, &one->vtransform.rz, 0.0f);

							nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * 3.6f);
							nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * 3.6f);

							nemotrans_set_float(trans, &one->color[0], 1.0f);
							nemotrans_set_float(trans, &one->color[1], 1.0f);
							nemotrans_set_float(trans, &one->color[2], 1.0f);
							nemotrans_set_float(trans, &one->color[3], 1.0f);

							nemotrans_group_attach_trans(tile->trans_group, trans);

							nemolist_remove(&one->link);
							nemolist_insert_tail(&tile->tile_list, &one->link);
						}
					}
				} else if (tile->slideshow == 3) {
					int csprites = tile->csprites;
					int rsprites = tile->rsprites;

					if (nemoshow_event_get_value(event) == KEY_LEFT) {
						csprites = CLAMP(tile->csprites - 1, 0, tile->columns - 1);
					} else if (nemoshow_event_get_value(event) == KEY_RIGHT) {
						csprites = CLAMP(tile->csprites + 1, 0, tile->columns - 1);
					} else if (nemoshow_event_get_value(event) == KEY_UP) {
						rsprites = CLAMP(tile->rsprites - 1, 0, tile->rows - 1);
					} else if (nemoshow_event_get_value(event) == KEY_DOWN) {
						rsprites = CLAMP(tile->rsprites + 1, 0, tile->rows + 1);
					}

					if (tile->csprites != csprites || tile->rsprites != rsprites) {
						tile->csprites = csprites;
						tile->rsprites = rsprites;

						trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 600, 0);

						nemotrans_set_float(trans, &tile->projection.tx, tile->csprites * -2.0f);
						nemotrans_set_float(trans, &tile->projection.ty, tile->rsprites * -2.0f);

						nemotrans_group_attach_trans(tile->trans_group, trans);
					}
				}
			}

			if (nemoshow_event_is_touch_down(show, event)) {
				tile->pone = nemotile_pick_simple(tile, nemoshow_event_get_x(event), nemoshow_event_get_y(event));
				if (tile->pone != NULL) {
					nemolist_remove(&tile->pone->link);
					nemolist_insert_tail(&tile->tile_list, &tile->pone->link);

					nemolist_for_each(one, &tile->tile_list, link) {
						if (one == tile->pone) {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
									random_get_int(600, 1200),
									random_get_int(30, 300));

							nemotrans_set_float(trans, &tile->pone->vtransform.sx, tile->pone->vtransform0.sx * 3.0f);
							nemotrans_set_float(trans, &tile->pone->vtransform.sy, tile->pone->vtransform0.sy * 3.0f);
						} else {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
									random_get_int(400, 800),
									random_get_int(60, 120));

							nemotrans_set_float(trans, &one->color[0], 0.12f);
							nemotrans_set_float(trans, &one->color[1], 0.12f);
							nemotrans_set_float(trans, &one->color[2], 0.12f);
							nemotrans_set_float(trans, &one->color[3], 0.12f);
						}

						nemotrans_group_attach_trans(tile->trans_group, trans);
					}
				}
			} else if (nemoshow_event_is_touch_up(show, event)) {
				if (tile->pone != NULL) {
					nemolist_for_each(one, &tile->tile_list, link) {
						if (one == tile->pone) {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
									random_get_int(600, 1200),
									random_get_int(30, 300));

							nemotrans_set_float(trans, &tile->pone->vtransform.sx, tile->pone->vtransform0.sx);
							nemotrans_set_float(trans, &tile->pone->vtransform.sy, tile->pone->vtransform0.sy);
						} else {
							trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
									random_get_int(400, 800),
									random_get_int(60, 120));

							nemotrans_set_float(trans, &one->color[0], 1.0f);
							nemotrans_set_float(trans, &one->color[1], 1.0f);
							nemotrans_set_float(trans, &one->color[2], 1.0f);
							nemotrans_set_float(trans, &one->color[3], 1.0f);
						}

						nemotrans_group_attach_trans(tile->trans_group, trans);
					}
				}
			}
		}
	} else {
		if (tile->is_dynamic_perspective == 0) {
			static float planes[6][6] = {
				{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
				{ 0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 0.0f },
				{ 0.0f, -1.0f, -1.0f, -M_PI / 2.0f, 0.0f, 0.0f },
				{ 0.0f, 1.0f, -1.0f, M_PI / 2.0f, 0.0f, 0.0f },
				{ -1.0f, 0.0f, -1.0f, 0.0f, M_PI / 2.0f, 0.0f },
				{ 1.0f, 0.0f, -1.0f, 0.0f, -M_PI / 2.0f, 0.0f },
			};

			if (nemoshow_event_is_pointer_left_down(show, event)) {
				int plane = random_get_int(1, 5);

				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(600, 1200), 0);

					nemotrans_set_float(trans, &one->gtransform.tz, planes[plane][2]);

					nemotrans_group_attach_trans(tile->trans_group, trans);

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(800, 1600),
							nemotrans_get_duration(trans) * 0.85f);

					nemotrans_set_float(trans, &one->gtransform.tx, planes[plane][0]);
					nemotrans_set_float(trans, &one->gtransform.ty, planes[plane][1]);
					nemotrans_set_float(trans, &one->gtransform.rx, planes[plane][3]);
					nemotrans_set_float(trans, &one->gtransform.ry, planes[plane][4]);
					nemotrans_set_float(trans, &one->gtransform.rz, planes[plane][5]);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (nemoshow_event_is_pointer_right_down(show, event)) {
				int plane = 0;

				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(800, 1600), 0);

					nemotrans_set_float(trans, &one->gtransform.tx, planes[plane][0]);
					nemotrans_set_float(trans, &one->gtransform.ty, planes[plane][1]);
					nemotrans_set_float(trans, &one->gtransform.rx, planes[plane][3]);
					nemotrans_set_float(trans, &one->gtransform.ry, planes[plane][4]);
					nemotrans_set_float(trans, &one->gtransform.rz, planes[plane][5]);

					nemotrans_group_attach_trans(tile->trans_group, trans);

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(600, 1200),
							nemotrans_get_duration(trans) * 0.85f);

					nemotrans_set_float(trans, &one->gtransform.tz, planes[plane][2]);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}
		} else {
			if (nemoshow_event_is_pointer_left_down(show, event)) {
				if (tile->ceil != NULL) {
					one = tile->ceil;

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 800, 0);

					nemotrans_set_float(trans, &one->gtransform.rx, 0.0f);

					nemotrans_set_float(trans, &one->color[0], 0.45f);
					nemotrans_set_float(trans, &one->color[1], 0.45f);
					nemotrans_set_float(trans, &one->color[2], 0.45f);
					nemotrans_set_float(trans, &one->color[3], 0.45f);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (nemoshow_event_is_pointer_left_up(show, event)) {
				if (tile->ceil != NULL) {
					one = tile->ceil;

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, 800, 0);

					nemotrans_set_float(trans, &one->gtransform.rx, M_PI / 2.0f);

					nemotrans_set_float(trans, &one->color[0], 0.025f);
					nemotrans_set_float(trans, &one->color[1], 0.025f);
					nemotrans_set_float(trans, &one->color[2], 0.025f);
					nemotrans_set_float(trans, &one->color[3], 0.025f);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (nemoshow_event_is_pointer_right_down(show, event)) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(800, 1600),
							random_get_int(100, 1200));
					nemotrans_set_tag(trans, 0x10001);

					nemotrans_set_float(trans, &one->vtransform.tz, 3.0f);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}

				nemotrans_group_ready(tile->trans_group, time_current_msecs());

				trans = nemotrans_group_get_last_tag(tile->trans_group, 0x10001);
				if (trans != NULL) {
					nemotrans_set_dispatch_done(trans, nemotile_dispatch_video_trans_done);
					nemotrans_set_userdata(trans, tile);

					nemotimer_set_timeout(tile->timer, tile->timeout);
				}
			} else if (nemoshow_event_is_pointer_motion(show, event)) {
				float tx = nemoshow_event_get_x(event) / tile->width;
				float ty = nemoshow_event_get_y(event) / tile->height;
				float rx, ry;

				tile->asymmetric.e[0] = tx * 3.0f - 1.5f;
				tile->asymmetric.e[1] = ty * 3.0f - 1.5f;

				rx = nemotile_get_camera_degree(tile->asymmetric.e[0], 2.0f);
				ry = nemotile_get_camera_degree(tile->asymmetric.e[1], 2.0f);
			}

			if (nemoshow_event_is_keyboard_down(show, event)) {
				trans = nemotrans_create(NEMOEASE_LINEAR_TYPE, 4500, 0);

				if (nemoshow_event_get_value(event) == KEY_LEFT) {
					nemotrans_set_float(trans, &tile->asymmetric.e[0], -1.5f);
				} else if (nemoshow_event_get_value(event) == KEY_RIGHT) {
					nemotrans_set_float(trans, &tile->asymmetric.e[0], 1.5f);
				} else if (nemoshow_event_get_value(event) == KEY_DOWN) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1600),
							random_get_int(200, 400));

					nemotrans_set_float(trans, &tile->projection.sz, 0.001f);
					nemotrans_set_float(trans, &tile->asymmetric.e[0], 0.0f);
					nemotrans_set_float(trans, &tile->asymmetric.e[1], 0.0f);
				} else if (nemoshow_event_get_value(event) == KEY_UP) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1600),
							random_get_int(200, 400));

					nemotrans_set_float(trans, &tile->projection.sz, 1.0f);
				}

				nemotrans_group_attach_trans(tile->trans_group, trans);
			}

			if (nemoshow_event_is_touch_down(show, event)) {
				float t, u, v;

				one = nemotile_pick_complex(tile, nemoshow_event_get_x(event), nemoshow_event_get_y(event), &t, &u, &v);
				if (one != NULL) {
				}
			}
		}
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_more_taps(show, event, 8)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}

	nemoshow_one_dirty(tile->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_frame(show);
}

static void nemotile_enter_show_frame(struct nemoshow *show, uint32_t msecs)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);

	nemotrans_group_dispatch(tile->trans_group, msecs);
}

static void nemotile_leave_show_frame(struct nemoshow *show, uint32_t msecs)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);
}

static void nemotile_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);

	nemoshow_view_resize(show, width, height);

	glDeleteFramebuffers(1, &tile->fbo);
	glDeleteRenderbuffers(1, &tile->dbo);

	gl_create_fbo(
			nemoshow_canvas_get_texture(tile->canvas),
			width, height,
			&tile->fbo, &tile->dbo);

	nemoshow_one_dirty(tile->canvas, NEMOSHOW_REDRAW_DIRTY);

	nemoshow_view_redraw(show);
}

static void nemotile_dispatch_sweep_trans_update(struct nemotrans *trans, void *data, float t)
{
	struct nemotile *tile = (struct nemotile *)data;

	nemofx_glsweep_set_timing(tile->sweep, t);
	nemofx_glsweep_set_rotate(tile->sweep, t * M_PI * 2.0f);
}

static void nemotile_dispatch_sweep_trans_done(struct nemotrans *trans, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;

	nemofx_glsweep_destroy(tile->sweep);
	tile->sweep = NULL;
}

static void nemotile_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;
	struct nemotrans *trans;
	struct tileone *one;
	float dx, dy;

	if (tile->is_3d == 0) {
		if (tile->is_single != 0) {
			if (tile->iactions == 0) {
				tile->flip = !tile->flip;

				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(700, 1400),
							random_get_int(100, 500));

					nemotrans_set_float(trans, &one->vtransform.rz, tile->flip == 0 ? 0.0f : M_PI);

					nemotrans_set_float(trans, &one->color[0], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[1], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[2], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[3], random_get_double(tile->brightness, 1.0f));

					nemotrans_set_float(trans, &one->ttransform.tx,
							0.5f - (one->ttransform.tx - 0.5f) - one->vtransform.sx);
					nemotrans_set_float(trans, &one->ttransform.ty,
							0.5f - (one->ttransform.ty - 0.5f) - one->vtransform.sy);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (tile->iactions == 1) {
				struct tileone *none;
				float tx, ty;
				float dtx, dty;
				float dsx, dsy;
				int index;

				nemolist_for_each(one, &tile->tile_list, link) {
					index = random_get_int(0, tile->columns * tile->rows - 1);

					none = nemotile_find_one(tile, index);
					if (none != one) {
						tx = one->vtransform0.tx;
						ty = one->vtransform0.ty;
						one->vtransform0.tx = none->vtransform0.tx;
						one->vtransform0.ty = none->vtransform0.ty;
						none->vtransform0.tx = tx;
						none->vtransform0.ty = ty;

						one->ttransform0.tx = nemotile_get_tx_from_vx(tile->columns, tile->flip, one->vtransform0.sx, one->vtransform0.tx);
						one->ttransform0.ty = nemotile_get_ty_from_vy(tile->rows, tile->flip, one->vtransform0.sy, one->vtransform0.ty);
						none->ttransform0.tx = nemotile_get_tx_from_vx(tile->columns, tile->flip, none->vtransform0.sx, none->vtransform0.tx);
						none->ttransform0.ty = nemotile_get_ty_from_vy(tile->rows, tile->flip, none->vtransform0.sy, none->vtransform0.ty);
					}
				}

				nemolist_for_each(one, &tile->tile_list, link) {
					dtx = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dty = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dsx = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dsy = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1800),
							random_get_int(200, 400));

					nemotrans_set_float(trans, &one->vtransform.tx, one->vtransform0.tx * dtx);
					nemotrans_set_float(trans, &one->vtransform.ty, one->vtransform0.ty * dty);
					nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * dsx);
					nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * dsy);

					nemotrans_group_attach_trans(tile->trans_group, trans);

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(400, 800),
							random_get_int(2400, 2800));

					nemotrans_set_float(trans, &one->ttransform.tx, nemotile_get_tx_from_vx(tile->columns, tile->flip, one->vtransform0.sx * dsx, one->vtransform0.tx * dtx));
					nemotrans_set_float(trans, &one->ttransform.ty, nemotile_get_ty_from_vy(tile->rows, tile->flip, one->vtransform0.sy * dsy, one->vtransform0.ty * dty));
					nemotrans_set_float(trans, &one->ttransform.sx, one->vtransform0.sx * dsx);
					nemotrans_set_float(trans, &one->ttransform.sy, one->vtransform0.sy * dsy);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}

			tile->iactions = (tile->iactions + 1) % 2;
		} else if (tile->slideshow == 0) {
			if (tile->iactions == 0) {
				tile->flip = !tile->flip;

				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(700, 1400),
							random_get_int(100, 500));

					nemotrans_set_float(trans, &one->vtransform.rz, tile->flip == 0 ? 0.0f : M_PI);

					nemotrans_set_float(trans, &one->color[0], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[1], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[2], random_get_double(tile->brightness, 1.0f));
					nemotrans_set_float(trans, &one->color[3], random_get_double(tile->brightness, 1.0f));

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (tile->iactions == 1) {
				struct tileone *none;
				float tx, ty;
				float dtx, dty;
				float dsx, dsy;
				int index;

				nemolist_for_each(one, &tile->tile_list, link) {
					index = random_get_int(0, tile->columns * tile->rows - 1);

					none = nemotile_find_one(tile, index);
					if (none != NULL && none != one) {
						tx = one->vtransform0.tx;
						ty = one->vtransform0.ty;
						one->vtransform0.tx = none->vtransform0.tx;
						one->vtransform0.ty = none->vtransform0.ty;
						none->vtransform0.tx = tx;
						none->vtransform0.ty = ty;
					}
				}

				nemolist_for_each(one, &tile->tile_list, link) {
					dtx = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dty = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dsx = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);
					dsy = random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter);

					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(1200, 1800),
							random_get_int(200, 400));

					nemotrans_set_float(trans, &one->vtransform.tx, one->vtransform0.tx * dtx);
					nemotrans_set_float(trans, &one->vtransform.ty, one->vtransform0.ty * dty);
					nemotrans_set_float(trans, &one->vtransform.sx, one->vtransform0.sx * dsx);
					nemotrans_set_float(trans, &one->vtransform.sy, one->vtransform0.sy * dsy);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}

			tile->iactions = (tile->iactions + 1) % 2;
		}
	}

	if (tile->is_3d != 0) {
		if (tile->is_dynamic_perspective == 0) {
			static float planes[6][6] = {
				{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
				{ 0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 0.0f },
				{ 0.0f, -1.0f, -1.0f, -M_PI / 2.0f, 0.0f, 0.0f },
				{ 0.0f, 1.0f, -1.0f, M_PI / 2.0f, 0.0f, 0.0f },
				{ -1.0f, 0.0f, -1.0f, 0.0f, M_PI / 2.0f, 0.0f },
				{ 1.0f, 0.0f, -1.0f, 0.0f, -M_PI / 2.0f, 0.0f },
			};
			int plane;

			nemolist_for_each(one, &tile->tile_list, link) {
				plane = random_get_int(1, 5);

				trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
						random_get_int(800, 1600),
						random_get_int(600, 1200));

				nemotrans_set_float(trans, &one->gtransform.tx, planes[plane][0]);
				nemotrans_set_float(trans, &one->gtransform.ty, planes[plane][1]);
				nemotrans_set_float(trans, &one->gtransform.tz, planes[plane][2]);
				nemotrans_set_float(trans, &one->gtransform.rx, planes[plane][3]);
				nemotrans_set_float(trans, &one->gtransform.ry, planes[plane][4]);
				nemotrans_set_float(trans, &one->gtransform.rz, planes[plane][5]);

				nemotrans_group_attach_trans(tile->trans_group, trans);
			}
		} else {
			if (tile->iactions == 0) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(700, 1400),
							random_get_int(100, 500));

					nemotrans_set_float(trans, &one->vtransform.tz, random_get_double(-0.25f, 0.45f));

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			} else if (tile->iactions == 1) {
				nemolist_for_each(one, &tile->tile_list, link) {
					trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
							random_get_int(700, 1400),
							random_get_int(100, 500));

					nemotrans_set_float(trans, &one->vtransform.ry, one->vtransform.ry + M_PI * 2.0f);

					nemotrans_group_attach_trans(tile->trans_group, trans);
				}
			}

			tile->iactions = (tile->iactions + 1) % 2;
		}

		if (tile->cube != NULL) {
			trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, tile->timeout, 0);
			nemotrans_set_float(trans, &tile->cube->vtransform.ry, tile->cube->vtransform.ry + M_PI * 2.0f);
			nemotrans_group_attach_trans(tile->trans_group, trans);
		}

		if (tile->mesh != NULL) {
			trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, tile->timeout, 0);
			nemotrans_set_float(trans, &tile->mesh->vtransform.ry, tile->mesh->vtransform.ry + M_PI * 2.0f);
			nemotrans_group_attach_trans(tile->trans_group, trans);
		}

		trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE, random_get_int(2400, 4800), 0);
		nemotrans_set_float(trans, &tile->light[0], random_get_double(-1.0f, 1.0f));
		nemotrans_set_float(trans, &tile->light[1], random_get_double(-1.0f, 1.0f));
		nemotrans_set_float(trans, &tile->light[2], random_get_double(0.0f, -2.0f));
		nemotrans_set_float(trans, &tile->light[3], random_get_double(1.0f, 2.0f));
		nemotrans_group_attach_trans(tile->trans_group, trans);
	}

	if (tile->filter != NULL && nemofs_dir_get_filecount(tile->shaders) >= 2) {
		tile->ishaders = (tile->ishaders + 1) % nemofs_dir_get_filecount(tile->shaders);

		nemofx_glfilter_set_program(tile->filter, nemofs_dir_get_filepath(tile->shaders, tile->ishaders));

		tile->sweep = nemofx_glsweep_create(tile->width, tile->height);
		nemofx_glsweep_set_snapshot(tile->sweep,
				nemoshow_canvas_get_effective_texture(tile->wall),
				nemoshow_canvas_get_viewport_width(tile->wall),
				nemoshow_canvas_get_viewport_height(tile->wall));
		nemofx_glsweep_set_type(tile->sweep, random_get_int(0, NEMOFX_GLSWEEP_LAST_TYPE - 1));
		nemofx_glsweep_set_timing(tile->sweep, 0.0f);
		nemofx_glsweep_set_point(tile->sweep, random_get_double(-0.5f, 0.5f), random_get_double(-0.5f, 0.5f));
		nemofx_glsweep_set_mask(tile->sweep, nemoshow_canvas_get_texture(tile->over));

		trans = nemotrans_create(NEMOEASE_CUBIC_OUT_TYPE, 1800, 0);
		nemotrans_set_dispatch_update(trans, nemotile_dispatch_sweep_trans_update);
		nemotrans_set_dispatch_done(trans, nemotile_dispatch_sweep_trans_done);
		nemotrans_set_userdata(trans, tile);
		nemotrans_group_attach_trans(tile->trans_group, trans);
	}

	nemotimer_set_timeout(tile->timer, tile->timeout);
}

static void nemotile_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;

	nemoshow_canvas_damage_all(tile->video);
	nemoshow_dispatch_frame(tile->show);
}

static void nemotile_dispatch_video_done(struct nemoplay *play, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;

	nemoplay_back_destroy_decoder(tile->decoderback);
	nemoplay_back_destroy_audio(tile->audioback);
	nemoplay_back_destroy_video(tile->videoback);
	nemoplay_destroy(tile->play);

	tile->imovies = (tile->imovies + 1) % nemofs_dir_get_filecount(tile->movies);

	tile->play = nemoplay_create();
	nemoplay_load_media(tile->play, nemofs_dir_get_filepath(tile->movies, tile->imovies));

	nemoshow_canvas_set_size(tile->video,
			nemoplay_get_video_width(tile->play),
			nemoplay_get_video_height(tile->play));

	tile->decoderback = nemoplay_back_create_decoder(tile->play);
	tile->audioback = nemoplay_back_create_audio_by_ao(tile->play);
	tile->videoback = nemoplay_back_create_video_by_timer(tile->play, tile->tool);
	nemoplay_back_set_video_texture(tile->videoback,
			nemoshow_canvas_get_texture(tile->video),
			nemoplay_get_video_width(tile->play),
			nemoplay_get_video_height(tile->play));
	nemoplay_back_set_video_update(tile->videoback, nemotile_dispatch_video_update);
	nemoplay_back_set_video_done(tile->videoback, nemotile_dispatch_video_done);
	nemoplay_back_set_video_data(tile->videoback, tile);
}

static uint32_t nemotile_dispatch_canvas_filter(void *node)
{
	struct showone *canvas = (struct showone *)node;
	struct nemotile *tile = (struct nemotile *)nemoshow_one_get_userdata(canvas);
	uint32_t texture = nemoshow_canvas_get_texture(canvas);

	if (tile->mask != NULL && tile->is_3d == 0)
		texture = nemofx_glmask_dispatch(tile->mask, texture, nemoshow_canvas_get_texture(tile->over));

	return texture;
}

static uint32_t nemotile_dispatch_video_filter(void *node)
{
	struct showone *canvas = (struct showone *)node;
	struct nemotile *tile = (struct nemotile *)nemoshow_one_get_userdata(canvas);
	uint32_t texture = nemoshow_canvas_get_texture(canvas);

	if (tile->polar != NULL) {
		nemofx_glpolar_resize(tile->polar,
				nemoplay_get_video_width(tile->play),
				nemoplay_get_video_height(tile->play));

		texture = nemofx_glpolar_dispatch(tile->polar, texture);
	}

	return texture;
}

static uint32_t nemotile_dispatch_wall_filter(void *node)
{
	struct showone *canvas = (struct showone *)node;
	struct nemotile *tile = (struct nemotile *)nemoshow_one_get_userdata(canvas);
	uint32_t texture = nemoshow_canvas_get_texture(canvas);

	if (tile->filter != NULL)
		texture = nemofx_glfilter_dispatch(tile->filter, texture);

	if (tile->sweep != NULL)
		texture = nemofx_glsweep_dispatch(tile->sweep, texture);

	return texture;
}

static int nemotile_prepare_opengl(struct nemotile *tile, int32_t width, int32_t height)
{
	static const char *vertexshader_texture =
		"uniform mat4 projection;\n"
		"uniform mat4 vtransform;\n"
		"uniform mat4 ttransform;\n"
		"attribute vec3 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec4 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * vtransform * vec4(position.xyz, 1.0);\n"
		"  vtexcoord = ttransform * vec4(texcoord.xy, 0.0, 1.0);\n"
		"}\n";
	static const char *fragmentshader_texture =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"uniform vec4 color;\n"
		"varying vec4 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord.xy) * color;\n"
		"}\n";
	static const char *vertexshader_texture_light =
		"uniform mat4 projection;\n"
		"uniform mat4 vtransform;\n"
		"uniform mat4 ttransform;\n"
		"uniform vec4 light;\n"
		"attribute vec3 position;\n"
		"attribute vec2 texcoord;\n"
		"attribute vec3 normal;\n"
		"varying vec4 vtexcoord;\n"
		"varying vec4 vnormal;\n"
		"varying vec4 vlight;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * vtransform * vec4(position.xyz, 1.0);\n"
		"  vlight = vec4(light.xyz, 1.0) - vtransform * vec4(position.xyz, 1.0);\n"
		"  vlight.w = light.w;\n"
		"  vtexcoord = ttransform * vec4(texcoord.xy, 0.0, 1.0);\n"
		"  vnormal = normalize(vtransform * vec4(normal, 1.0));\n"
		"}\n";
	static const char *fragmentshader_texture_light =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"uniform vec4 ambient;\n"
		"varying vec4 vtexcoord;\n"
		"varying vec4 vnormal;\n"
		"varying vec4 vlight;\n"
		"void main()\n"
		"{\n"
		"  float v = abs(dot(normalize(vlight.xyz), vnormal.xyz));\n"
		"  float f = length(vlight.xyz);\n"
		"  vec4 t = texture2D(texture, vtexcoord.xy);\n"
		"  gl_FragColor.rgb = t.rgb * ambient.rgb * t.a + t.rgb * vec3(v, v, v) * vlight.w / f / f / f * t.a;\n"
		"  gl_FragColor.a = t.a;\n"
		"}\n";
	static const char *vertexshader_diffuse =
		"uniform mat4 projection;\n"
		"uniform mat4 vtransform;\n"
		"attribute vec3 position;\n"
		"attribute vec4 diffuse;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * vtransform * vec4(position.xyz, 1.0);\n"
		"  vdiffuse = diffuse;\n"
		"}\n";
	static const char *fragmentshader_diffuse =
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vdiffuse * color;\n"
		"}\n";
	static const char *vertexshader_diffuse_light =
		"uniform mat4 projection;\n"
		"uniform mat4 vtransform;\n"
		"uniform vec4 light;\n"
		"attribute vec3 position;\n"
		"attribute vec4 diffuse;\n"
		"attribute vec3 normal;\n"
		"varying vec4 vdiffuse;\n"
		"varying vec4 vnormal;\n"
		"varying vec4 vlight;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = projection * vtransform * vec4(position.xyz, 1.0);\n"
		"  vlight = vec4(light.xyz, 1.0) - vtransform * vec4(position.xyz, 1.0);\n"
		"  vlight.w = light.w;\n"
		"  vdiffuse = diffuse;\n"
		"  vnormal = normalize(vtransform * vec4(normal, 1.0));\n"
		"}\n";
	static const char *fragmentshader_diffuse_light =
		"precision mediump float;\n"
		"uniform vec4 ambient;\n"
		"varying vec4 vdiffuse;\n"
		"varying vec4 vnormal;\n"
		"varying vec4 vlight;\n"
		"void main()\n"
		"{\n"
		"  float v = abs(dot(normalize(vlight.xyz), vnormal.xyz));\n"
		"  float f = length(vlight.xyz);\n"
		"  vec4 t = vdiffuse;\n"
		"  gl_FragColor.rgb = t.rgb * ambient.rgb * t.a + t.rgb * vec3(v, v, v) * vlight.w / f / f / f * t.a;\n"
		"  gl_FragColor.a = t.a;\n"
		"}\n";

	gl_create_fbo(
			nemoshow_canvas_get_texture(tile->canvas),
			width, height,
			&tile->fbo, &tile->dbo);

	tile->programs[0] = gl_compile_program(vertexshader_texture, fragmentshader_texture, NULL, NULL);
	tile->programs[1] = gl_compile_program(vertexshader_texture_light, fragmentshader_texture_light, NULL, NULL);
	tile->programs[2] = gl_compile_program(vertexshader_diffuse, fragmentshader_diffuse, NULL, NULL);
	tile->programs[3] = gl_compile_program(vertexshader_diffuse_light, fragmentshader_diffuse_light, NULL, NULL);

	tile->uprojection0 = glGetUniformLocation(tile->programs[0], "projection");
	tile->uvtransform0 = glGetUniformLocation(tile->programs[0], "vtransform");
	tile->uttransform0 = glGetUniformLocation(tile->programs[0], "ttransform");
	tile->utexture0 = glGetUniformLocation(tile->programs[0], "texture");
	tile->ucolor0 = glGetUniformLocation(tile->programs[0], "color");

	tile->uprojection1 = glGetUniformLocation(tile->programs[1], "projection");
	tile->uvtransform1 = glGetUniformLocation(tile->programs[1], "vtransform");
	tile->uttransform1 = glGetUniformLocation(tile->programs[1], "ttransform");
	tile->utexture1 = glGetUniformLocation(tile->programs[1], "texture");
	tile->uambient1 = glGetUniformLocation(tile->programs[1], "ambient");
	tile->ulight1 = glGetUniformLocation(tile->programs[1], "light");

	tile->uprojection2 = glGetUniformLocation(tile->programs[2], "projection");
	tile->uvtransform2 = glGetUniformLocation(tile->programs[2], "vtransform");
	tile->ucolor2 = glGetUniformLocation(tile->programs[2], "color");

	tile->uprojection3 = glGetUniformLocation(tile->programs[3], "projection");
	tile->uvtransform3 = glGetUniformLocation(tile->programs[3], "vtransform");
	tile->uambient3 = glGetUniformLocation(tile->programs[3], "ambient");
	tile->ulight3 = glGetUniformLocation(tile->programs[3], "light");

	return 0;
}

static void nemotile_finish_opengl(struct nemotile *tile)
{
	glDeleteFramebuffers(1, &tile->fbo);
	glDeleteRenderbuffers(1, &tile->dbo);

	glDeleteProgram(tile->programs[0]);
	glDeleteProgram(tile->programs[1]);
	glDeleteProgram(tile->programs[2]);
	glDeleteProgram(tile->programs[3]);
}

static int nemotile_prepare_image(struct nemotile *tile, int columns, int rows, float padding)
{
	struct tileone *one;
	int index = 0;
	int x, y;

	for (y = 0; y < rows && index < tile->nsprites; y++) {
		for (x = 0; x < columns && index < tile->nsprites; x++, index++) {
			one = nemotile_one_create(4);
			nemotile_one_set_index(one, (y * columns) + x);
			nemotile_one_set_vertex(one, 0, -1.0f, 1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
			nemotile_one_set_vertex(one, 1, 1.0f, 1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
			nemotile_one_set_vertex(one, 2, -1.0f, -1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 2, 0.0f, 0.0f);
			nemotile_one_set_vertex(one, 3, 1.0f, -1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 3, 1.0f, 0.0f);

			nemotile_one_set_normal(one, 0, 0.0f, 0.0f, 1.0f);
			nemotile_one_set_normal(one, 1, 0.0f, 0.0f, 1.0f);
			nemotile_one_set_normal(one, 2, 0.0f, 0.0f, 1.0f);
			nemotile_one_set_normal(one, 3, 0.0f, 0.0f, 1.0f);

			nemotile_one_set_texture(one, tile->sprites[index]);

			nemotile_one_set_color(one,
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f));

			nemotile_one_vertices_translate_to(one,
					nemotile_get_column_x(columns, x),
					nemotile_get_row_y(rows, y),
					0.0f);
			nemotile_one_vertices_scale_to(one,
					nemotile_get_column_sx(columns) * (1.0f - padding),
					nemotile_get_row_sy(rows) * (1.0f - padding),
					1.0f);
			nemotile_one_vertices_translate(one,
					nemotile_get_column_x(columns, x) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					nemotile_get_row_y(rows, y) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					0.0f);
			nemotile_one_vertices_scale(one,
					nemotile_get_column_sx(columns) * (1.0f - padding) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					nemotile_get_row_sy(rows) * (1.0f - padding) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					1.0f);

			nemolist_insert_tail(&tile->tile_list, &one->link);
		}
	}

	return 0;
}

static int nemotile_prepare_video(struct nemotile *tile, int columns, int rows, float padding)
{
	struct tileone *one;
	int x, y;

	for (y = 0; y < rows; y++) {
		for (x = 0; x < columns; x++) {
			one = nemotile_one_create(4);
			nemotile_one_set_index(one, (y * columns) + x);
			nemotile_one_set_vertex(one, 0, -1.0f, 1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
			nemotile_one_set_vertex(one, 1, 1.0f, 1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
			nemotile_one_set_vertex(one, 2, -1.0f, -1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 2, 0.0f, 0.0f);
			nemotile_one_set_vertex(one, 3, 1.0f, -1.0f, 0.0f);
			nemotile_one_set_texcoord(one, 3, 1.0f, 0.0f);

			nemotile_one_set_normal(one, 0, 0.0f, 0.0f, 1.0f);
			nemotile_one_set_normal(one, 1, 0.0f, 0.0f, 1.0f);
			nemotile_one_set_normal(one, 2, 0.0f, 0.0f, 1.0f);
			nemotile_one_set_normal(one, 3, 0.0f, 0.0f, 1.0f);

			nemotile_one_set_texture(one, tile->video);

			nemotile_one_set_color(one,
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f));

			nemotile_one_vertices_translate_to(one,
					nemotile_get_column_x(columns, x),
					nemotile_get_row_y(rows, y),
					0.0f);
			nemotile_one_vertices_scale_to(one,
					nemotile_get_column_sx(columns) * (1.0f - padding),
					nemotile_get_row_sy(rows) * (1.0f - padding),
					1.0f);
			nemotile_one_vertices_translate(one,
					nemotile_get_column_x(columns, x) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					nemotile_get_row_y(rows, y) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					0.0f);
			nemotile_one_vertices_scale(one,
					nemotile_get_column_sx(columns) * (1.0f - padding) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					nemotile_get_row_sy(rows) * (1.0f - padding) * random_get_double(1.0f - tile->jitter, 1.0f + tile->jitter),
					1.0f);

			nemotile_one_group_translate_to(one, 0.0f, 0.0f, -1.75f);
			nemotile_one_group_translate(one, 0.0f, 0.0f, -1.75f);

			nemotile_one_texcoords_translate_to(one,
					nemotile_get_tx_from_vx(tile->columns, 0, one->vtransform.sx, one->vtransform0.tx),
					nemotile_get_ty_from_vy(tile->rows, 0, one->vtransform.sy, one->vtransform0.ty));
			nemotile_one_texcoords_scale_to(one,
					one->vtransform0.sx,
					one->vtransform0.sy);
			nemotile_one_texcoords_translate(one,
					nemotile_get_tx_from_vx(tile->columns, 0, one->vtransform.sx, one->vtransform.tx),
					nemotile_get_ty_from_vy(tile->rows, 0, one->vtransform.sy, one->vtransform.ty));
			nemotile_one_texcoords_scale(one,
					one->vtransform.sx,
					one->vtransform.sy);

			nemolist_insert_tail(&tile->tile_list, &one->link);
		}
	}

	return 0;
}

static int nemotile_prepare_depthtest(struct nemotile *tile, int count, float padding)
{
	struct tileone *one;
	int i;

	for (i = 0; i < count; i++) {
		one = nemotile_one_create(4);
		nemotile_one_set_index(one, i);
		nemotile_one_set_vertex(one, 0, -1.0f, 1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
		nemotile_one_set_vertex(one, 1, 1.0f, 1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
		nemotile_one_set_vertex(one, 2, -1.0f, -1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 2, 0.0f, 0.0f);
		nemotile_one_set_vertex(one, 3, 1.0f, -1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 3, 1.0f, 0.0f);

		nemotile_one_set_normal(one, 0, 0.0f, 0.0f, 1.0f);
		nemotile_one_set_normal(one, 1, 0.0f, 0.0f, 1.0f);
		nemotile_one_set_normal(one, 2, 0.0f, 0.0f, 1.0f);
		nemotile_one_set_normal(one, 3, 0.0f, 0.0f, 1.0f);

		nemotile_one_set_texture(one, tile->test);

		nemotile_one_set_color(one,
				random_get_double(0.0f, 0.0f),
				random_get_double(0.15f, 0.65f),
				random_get_double(0.15f, 0.65f),
				random_get_double(0.15f, 0.65f));

		nemotile_one_vertices_translate(one,
				random_get_double(-0.25f, 0.25f),
				random_get_double(-0.25f, 0.25f),
				0.9f - 0.1f * i);
		nemotile_one_vertices_scale(one,
				random_get_double(0.04f, 0.08f),
				random_get_double(0.04f, 0.08f),
				1.0f);

		nemolist_insert(&tile->test_list, &one->link);
	}

	return 0;
}

static int nemotile_prepare_wall(struct nemotile *tile, float padding, int is_united_wall)
{
	static float vertices0[5][6] = {
		{ 0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, -1.0f, -1.0f, M_PI / 2.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, -1.0f, M_PI / 2.0f, 0.0f, -M_PI - M_PI / 2.0f },
		{ 0.0f, 1.0f, -1.0f, M_PI / 2.0f, 0.0f, M_PI },
		{ -1.0f, 0.0f, -1.0f, M_PI / 2.0f, 0.0f, -M_PI / 2.0f },
	};
	static float texcoords0[5][6] = {
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 1.0f, 1.0f }
	};
	static float vertices1[5][6] = {
		{ 0.0f, 0.0f, -2.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, -1.0f, -1.0f, M_PI / 2.0f, 0.0f, 0.0f },
		{ 1.0f, 0.0f, -1.0f, M_PI / 2.0f, 0.0f, -M_PI - M_PI / 2.0f },
		{ 0.0f, 1.0f, -1.0f, M_PI / 2.0f, 0.0f, M_PI },
		{ -1.0f, 0.0f, -1.0f, M_PI / 2.0f, 0.0f, -M_PI / 2.0f },
	};
	static float texcoords1[5][6] = {
		{ 0.0f, 0.0f, 1.0f, 1.0f },
		{ 0.0f, 0.0f, 0.25f, 1.0f },
		{ 0.25f, 0.0f, 0.25f, 1.0f },
		{ 0.50f, 0.0f, 0.25f, 1.0f },
		{ 0.75f, 0.0f, 0.25f, 1.0f }
	};
	float (*vertices)[6] = (is_united_wall == 0 ? vertices0 : vertices1);
	float (*texcoords)[6] = (is_united_wall == 0 ? texcoords0 : texcoords1);
	struct tileone *one;
	int i;

	for (i = 0; i < 5; i++) {
		one = nemotile_one_create(4);
		nemotile_one_set_index(one, i);
		nemotile_one_set_vertex(one, 0, -1.0f, 1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
		nemotile_one_set_vertex(one, 1, 1.0f, 1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
		nemotile_one_set_vertex(one, 2, -1.0f, -1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 2, 0.0f, 0.0f);
		nemotile_one_set_vertex(one, 3, 1.0f, -1.0f, 0.0f);
		nemotile_one_set_texcoord(one, 3, 1.0f, 0.0f);

		nemotile_one_set_normal(one, 0, 0.0f, 0.0f, 1.0f);
		nemotile_one_set_normal(one, 1, 0.0f, 0.0f, 1.0f);
		nemotile_one_set_normal(one, 2, 0.0f, 0.0f, 1.0f);
		nemotile_one_set_normal(one, 3, 0.0f, 0.0f, 1.0f);

		if (i == 0 && tile->rear != NULL)
			nemotile_one_set_texture(one, tile->rear);
		else
			nemotile_one_set_texture(one, tile->wall);

		nemotile_one_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);

		nemotile_one_vertices_translate(one, vertices[i][0], vertices[i][1], vertices[i][2]);
		nemotile_one_vertices_rotate(one, vertices[i][3], vertices[i][4], vertices[i][5]);
		nemotile_one_vertices_scale(one, 1.0f - padding, 1.0f - padding, 1.0f);

		nemotile_one_texcoords_translate(one, texcoords[i][0], texcoords[i][1]);
		nemotile_one_texcoords_scale(one, texcoords[i][2], texcoords[i][3]);

		nemolist_insert_tail(&tile->wall_list, &one->link);
	}

	return 0;
}

static int nemotile_prepare_cube(struct nemotile *tile)
{
	static const float vertices[] = {
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,

		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,

		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,

		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f
	};
	static const float texcoords[] = {
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	static const float normals[] = {
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, -1.0f,

		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, -1.0f, 0.0f,

		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f,

		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f,

		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
		0.0f, 1.0f, 0.0f,

		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f
	};
	struct tileone *one;
	int i;

	tile->cube = one = nemotile_one_create(36);
	nemotile_one_set_index(one, 0);
	nemotile_one_set_type(one, GL_TRIANGLES);

	for (i = 0; i < 36; i++) {
		nemotile_one_set_vertex(one, i, vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]);
		nemotile_one_set_texcoord(one, i, texcoords[i * 2 + 0], texcoords[i * 2 + 1]);
		nemotile_one_set_normal(one, i, normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]);
	}

	nemotile_one_set_texture(one, tile->over);

	nemotile_one_set_color(one, 1.0f, 1.0f, 1.0f, 1.0f);

	nemotile_one_vertices_translate(one, 0.0f, 0.5f, 0.25f);
	nemotile_one_vertices_scale(one, 0.25f, 0.25f, 0.25f);

	return 0;
}

static int nemotile_prepare_ceil(struct nemotile *tile)
{
	struct tileone *one;

	tile->ceil = one = nemotile_one_create(4);
	nemotile_one_set_index(one, 0);
	nemotile_one_set_vertex(one, 0, -1.0f, 1.0f, 0.0f);
	nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
	nemotile_one_set_vertex(one, 1, 1.0f, 1.0f, 0.0f);
	nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
	nemotile_one_set_vertex(one, 2, -1.0f, -1.0f, 0.0f);
	nemotile_one_set_texcoord(one, 2, 0.0f, 0.0f);
	nemotile_one_set_vertex(one, 3, 1.0f, -1.0f, 0.0f);
	nemotile_one_set_texcoord(one, 3, 1.0f, 0.0f);

	nemotile_one_set_normal(one, 0, 0.0f, 0.0f, 1.0f);
	nemotile_one_set_normal(one, 1, 0.0f, 0.0f, 1.0f);
	nemotile_one_set_normal(one, 2, 0.0f, 0.0f, 1.0f);
	nemotile_one_set_normal(one, 3, 0.0f, 0.0f, 1.0f);

	nemotile_one_set_texture(one, tile->over);

	nemotile_one_set_color(one, 0.025f, 0.025f, 0.025f, 0.025f);

	nemotile_one_group_translate(one, -0.5f, 1.0f, 0.0f);
	nemotile_one_group_rotate(one, M_PI / 2.0f, 0.0f, 0.0f);

	nemotile_one_vertices_translate(one, 0.0f, -0.5f, 0.0f);
	nemotile_one_vertices_rotate(one, 0.0f, 0.0f, 0.0f);
	nemotile_one_vertices_scale(one, 0.5f, 0.5f, 0.5f);

	return 0;
}

static void nemotile_finish_tiles(struct nemotile *tile)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",									required_argument,			NULL,			'w' },
		{ "height",									required_argument,			NULL,			'h' },
		{ "columns",								required_argument,			NULL,			'c' },
		{ "rows",										required_argument,			NULL,			'r' },
		{ "image",									required_argument,			NULL,			'i' },
		{ "video",									required_argument,			NULL,			'v' },
		{ "timeout",								required_argument,			NULL,			't' },
		{ "background",							required_argument,			NULL,			'b' },
		{ "overlay",								required_argument,			NULL,			'o' },
		{ "wall",										required_argument,			NULL,			'a' },
		{ "mesh",										required_argument,			NULL,			'm' },
		{ "fullscreen",							required_argument,			NULL,			'f' },
		{ "brightness",							required_argument,			NULL,			'e' },
		{ "jitter",									required_argument,			NULL,			'j' },
		{ "padding",								required_argument,			NULL,			'd' },
		{ "program",								required_argument,			NULL,			'p' },
		{ "polar",									required_argument,			NULL,			'k' },
		{ "depthtest",							required_argument,			NULL,			'z' },
		{ "3dspace",								no_argument,						NULL,			's' },
		{ "lighting",								no_argument,						NULL,			'l' },
		{ "dynamic_perspective",		no_argument,						NULL,			'y' },
		{ "wall_divide",						no_argument,						NULL,			'u' },
		{ 0 }
	};

	struct nemotile *tile;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showone *blur;
	struct showtransition *trans;
	char *imagepath = NULL;
	char *videopath = NULL;
	char *programpath = NULL;
	char *polarcolor = NULL;
	char *fullscreen = NULL;
	char *backgroundpath = NULL;
	char *overlaypath = NULL;
	char *wallpath = NULL;
	char *meshpath = NULL;
	char *depthtest = NULL;
	float brightness = 0.85f;
	float jitter = 0.0f;
	float padding = 0.0f;
	int timeout = 5000;
	int width = 800;
	int height = 800;
	int columns = 1;
	int rows = 1;
	int fps = 60;
	int is_3d = 0;
	int is_lighting = 0;
	int is_dynamic_perspective = 0;
	int is_united_wall = 0;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:c:r:i:v:t:b:o:a:m:f:e:j:d:p:k:z:slyu", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				columns = strtoul(optarg, NULL, 10);
				break;

			case 'r':
				rows = strtoul(optarg, NULL, 10);
				break;

			case 'i':
				imagepath = strdup(optarg);
				break;

			case 'v':
				videopath = strdup(optarg);
				break;

			case 't':
				timeout = strtoul(optarg, NULL, 10);
				break;

			case 'b':
				backgroundpath = strdup(optarg);
				break;

			case 'o':
				overlaypath = strdup(optarg);
				break;

			case 'a':
				wallpath = strdup(optarg);
				break;

			case 'm':
				meshpath = strdup(optarg);
				break;

			case 'f':
				fullscreen = strdup(optarg);
				break;

			case 'e':
				brightness = strtod(optarg, NULL);
				break;

			case 'j':
				jitter = strtod(optarg, NULL);
				break;

			case 'd':
				padding = strtod(optarg, NULL);
				break;

			case 'p':
				programpath = strdup(optarg);
				break;

			case 'k':
				polarcolor = strdup(optarg);
				break;

			case 'z':
				depthtest = strdup(optarg);
				break;

			case 's':
				is_3d = 1;
				break;

			case 'l':
				is_lighting = 1;
				break;

			case 'y':
				is_dynamic_perspective = 1;
				break;

			case 'u':
				is_united_wall = 1;
				break;

			default:
				break;
		}
	}

	tile = (struct nemotile *)malloc(sizeof(struct nemotile));
	if (tile == NULL)
		goto err1;
	memset(tile, 0, sizeof(struct nemotile));

	tile->width = width;
	tile->height = height;
	tile->columns = columns;
	tile->rows = rows;
	tile->timeout = timeout;
	tile->brightness = brightness;
	tile->jitter = jitter;
	tile->is_3d = is_3d;
	tile->is_lighting = is_lighting;
	tile->is_dynamic_perspective = is_dynamic_perspective;

	if (is_dynamic_perspective == 0) {
		tile->projection.tx = 0.0f;
		tile->projection.ty = 0.0f;
		tile->projection.tz = -1.0f;
		tile->projection.rx = 0.0f;
		tile->projection.ry = 0.0f;
		tile->projection.rz = 0.0f;
		tile->projection.sx = 1.0f;
		tile->projection.sy = 1.0f;
		tile->projection.sz = 0.5f;

		tile->perspective.left = -1.0f;
		tile->perspective.right = 1.0f;
		tile->perspective.bottom = -1.0f;
		tile->perspective.top = 1.0f;
		tile->perspective.near = 0.99999f;
		tile->perspective.far = 10.0f;
	} else {
		tile->projection.tx = 0.0f;
		tile->projection.ty = 0.0f;
		tile->projection.tz = 0.0f;
		tile->projection.rx = 0.0f;
		tile->projection.ry = 0.0f;
		tile->projection.rz = 0.0f;
		tile->projection.sx = 1.0f;
		tile->projection.sy = 1.0f;
		tile->projection.sz = 1.0f;

		tile->asymmetric.a[0] = -1.0f;
		tile->asymmetric.a[1] = -1.0f;
		tile->asymmetric.a[2] = 0.0f;
		tile->asymmetric.b[0] = 1.0f;
		tile->asymmetric.b[1] = -1.0f;
		tile->asymmetric.b[2] = 0.0f;
		tile->asymmetric.c[0] = -1.0f;
		tile->asymmetric.c[1] = 1.0f;
		tile->asymmetric.c[2] = 0.0f;
		tile->asymmetric.e[0] = 0.0f;
		tile->asymmetric.e[1] = 0.0f;
		tile->asymmetric.e[2] = 1.0f;
		tile->asymmetric.near = 0.00001f;
		tile->asymmetric.far = 10.0f;
	}

	tile->light[0] = -0.98f;
	tile->light[1] = -0.98f;
	tile->light[2] = -1.98f;
	tile->light[3] = 1.2f;

	tile->ambient[0] = 0.12f;
	tile->ambient[1] = 0.12f;
	tile->ambient[2] = 0.12f;
	tile->ambient[3] = 0.12f;

	nemolist_init(&tile->tile_list);
	nemolist_init(&tile->wall_list);
	nemolist_init(&tile->test_list);

	tile->trans_group = nemotrans_group_create();

	tile->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	tile->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_filtering_quality(show, NEMOSHOW_FILTER_HIGH_QUALITY);
	nemoshow_set_enter_frame(show, nemotile_enter_show_frame);
	nemoshow_set_leave_frame(show, nemotile_leave_show_frame);
	nemoshow_set_dispatch_resize(show, nemotile_dispatch_show_resize);
	nemoshow_set_userdata(show, tile);

	nemoshow_view_set_framerate(show, fps);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	tile->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	tile->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	tile->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemotile_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemotile_dispatch_canvas_event);
	nemoshow_canvas_set_dispatch_filter(canvas, nemotile_dispatch_canvas_filter);
	nemoshow_one_set_userdata(canvas, tile);
	nemoshow_one_attach(scene, canvas);

	if (programpath != NULL) {
		if (os_check_path_is_directory(programpath) != 0) {
			tile->shaders = nemofs_dir_create(programpath, 32);
			nemofs_dir_scan_extension(tile->shaders, "fsh");
		} else {
			tile->shaders = nemofs_dir_create(NULL, 32);
			nemofs_dir_insert_file(tile->shaders, programpath);
		}

		tile->filter = nemofx_glfilter_create(width, height);
		nemofx_glfilter_set_program(tile->filter, nemofs_dir_get_filepath(tile->shaders, tile->ishaders));
	}

	if (polarcolor != NULL) {
		uint32_t color = color_parse(polarcolor);

		tile->polar = nemofx_glpolar_create(width, height);
		nemofx_glpolar_set_color(tile->polar,
				COLOR_DOUBLE_R(color),
				COLOR_DOUBLE_G(color),
				COLOR_DOUBLE_B(color),
				COLOR_DOUBLE_A(color));
	}

	if (backgroundpath != NULL) {
		tile->rear = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_attach_one(show, canvas);

		if (os_has_file_extension(backgroundpath, "svg") != 0) {
			blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
			nemoshow_filter_set_blur(blur, "solid", width * 0.05f);

			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_fill_color(one, 255.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_set_filter(one, blur);
			nemoshow_item_path_load_svg(one, backgroundpath, 0.0f, 0.0f, width, height);
		} else {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_uri(one, backgroundpath);
		}
	}

	if (overlaypath != NULL) {
		tile->over = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_attach_one(show, canvas);

		if (os_has_file_extension(overlaypath, "svg") != 0) {
			blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
			nemoshow_filter_set_blur(blur, "solid", width * 0.05f);

			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_fill_color(one, 255.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_set_filter(one, blur);
			nemoshow_item_path_load_svg(one, overlaypath, 0.0f, 0.0f, width, height);
		} else {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_uri(one, overlaypath);
		}

		tile->mask = nemofx_glmask_create(width, height);
	}

	if (wallpath != NULL) {
		tile->wall = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_canvas_set_dispatch_filter(canvas, nemotile_dispatch_wall_filter);
		nemoshow_one_set_userdata(canvas, tile);
		nemoshow_attach_one(show, canvas);

		if (os_has_file_extension(wallpath, "svg") != 0) {
			blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
			nemoshow_filter_set_blur(blur, "solid", width * 0.008f);

			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_stroke_color(one, 0.0f, 255.0f * 0.85f, 255.0f * 0.85f, 255.0f * 0.85f);
			nemoshow_item_set_stroke_width(one, width * 0.004f);
			nemoshow_item_set_fill_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
			nemoshow_item_set_filter(one, blur);
			nemoshow_item_path_load_svg(one, wallpath, 0.0f, 0.0f, width, height);
		} else {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_uri(one, wallpath);
		}

		trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
		nemoshow_transition_dirty_one(trans, canvas, NEMOSHOW_FILTER_DIRTY);
		nemoshow_transition_set_repeat(trans, 0);
		nemoshow_attach_transition(show, trans);
	}

	if (meshpath != NULL) {
		tile->mesh = nemotile_one_create_polyline(meshpath, os_get_file_path(meshpath));
		nemotile_one_vertices_scale_to(tile->mesh, 0.25f, -0.25f, 0.25f);
		nemotile_one_vertices_scale(tile->mesh, 0.25f, -0.25f, 0.25f);
		nemotile_one_vertices_translate_to(tile->mesh, 0.0f, 0.0f, 0.5f);
		nemotile_one_vertices_translate(tile->mesh, 0.0f, 0.0f, 0.5f);
	}

	if (depthtest != NULL) {
		tile->test = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_attach_one(show, canvas);

		if (os_has_file_extension(depthtest, "svg") != 0) {
			blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
			nemoshow_filter_set_blur(blur, "solid", width * 0.05f);

			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_fill_color(one, 255.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_set_filter(one, blur);
			nemoshow_item_path_load_svg(one, depthtest, 0.0f, 0.0f, width, height);
		} else {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_uri(one, depthtest);
		}
	}

	nemotile_prepare_opengl(tile, width, height);

	if (imagepath != NULL) {
		if (os_check_path_is_directory(imagepath) != 0) {
			struct fsdir *dir;
			const char *filepath;
			int i;

			dir = nemofs_dir_create(imagepath, 32);
			nemofs_dir_scan_extension(dir, "svg");

			for (i = 0; i < nemofs_dir_get_filecount(dir); i++) {
				srand(time_current_msecs());

				filepath = nemofs_dir_get_filepath(dir, i);

				tile->sprites[tile->nsprites++] = canvas = nemoshow_canvas_create();
				nemoshow_canvas_set_width(canvas, width);
				nemoshow_canvas_set_height(canvas, height);
				nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
				nemoshow_attach_one(show, canvas);

				one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, width);
				nemoshow_item_set_height(one, height);
				nemoshow_item_set_fill_color(one,
						random_get_double(0.0f, 128.0f),
						random_get_double(0.0f, 128.0f),
						random_get_double(0.0f, 128.0f),
						255.0f);
				nemoshow_item_set_stroke_color(one,
						random_get_double(128.0f, 255.0f),
						random_get_double(128.0f, 255.0f),
						random_get_double(128.0f, 255.0f),
						255.0f);
				nemoshow_item_set_stroke_width(one, width / 128);
				nemoshow_item_path_load_svg(one, filepath, 0.0f, 0.0f, width, height);

				nemoshow_update_one(show);
				nemoshow_canvas_render(show, canvas);
			}

			nemofs_dir_clear(dir);
			nemofs_dir_scan_extension(dir, "png");
			nemofs_dir_scan_extension(dir, "jpg");

			for (i = 0; i < nemofs_dir_get_filecount(dir); i++) {
				filepath = nemofs_dir_get_filepath(dir, i);

				tile->sprites[tile->nsprites++] = canvas = nemoshow_canvas_create();
				nemoshow_canvas_set_width(canvas, width);
				nemoshow_canvas_set_height(canvas, height);
				nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
				nemoshow_attach_one(show, canvas);

				one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, width);
				nemoshow_item_set_height(one, height);
				nemoshow_item_set_uri(one, filepath);

				nemoshow_update_one(show);
				nemoshow_canvas_render(show, canvas);
			}

			nemofs_dir_destroy(dir);

			tile->nsprites = MIN(tile->nsprites, columns * rows);
		} else {
			tile->sprites[0] = canvas = nemoshow_canvas_create();
			nemoshow_canvas_set_width(canvas, width);
			nemoshow_canvas_set_height(canvas, height);
			nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
			nemoshow_attach_one(show, canvas);

			tile->nsprites = 1;
			tile->isprites = 0;

			if (os_has_file_extension(imagepath, "svg") != 0) {
				one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, width);
				nemoshow_item_set_height(one, height);
				nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
				nemoshow_item_path_load_svg(one, imagepath, 0.0f, 0.0f, width, height);
			} else if (os_has_file_extensions(imagepath, "png", "jpg", NULL) != 0) {
				one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
				nemoshow_one_attach(canvas, one);
				nemoshow_item_set_x(one, 0.0f);
				nemoshow_item_set_y(one, 0.0f);
				nemoshow_item_set_width(one, width);
				nemoshow_item_set_height(one, height);
				nemoshow_item_set_uri(one, imagepath);
			}

			nemoshow_update_one(show);
			nemoshow_canvas_render(show, canvas);
		}

		nemotile_prepare_image(tile, columns, rows, padding);
	}

	if (videopath != NULL) {
		if (os_check_path_is_directory(videopath) != 0) {
			tile->movies = nemofs_dir_create(videopath, 32);
			nemofs_dir_scan_extension(tile->movies, "mp4");
			nemofs_dir_scan_extension(tile->movies, "avi");
			nemofs_dir_scan_extension(tile->movies, "ts");
		} else {
			tile->movies = nemofs_dir_create(NULL, 32);
			nemofs_dir_insert_file(tile->movies, videopath);
		}

		tile->play = nemoplay_create();
		nemoplay_load_media(tile->play, nemofs_dir_get_filepath(tile->movies, tile->imovies));

		tile->video = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, nemoplay_get_video_width(tile->play));
		nemoshow_canvas_set_height(canvas, nemoplay_get_video_height(tile->play));
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
		nemoshow_canvas_set_dispatch_filter(canvas, nemotile_dispatch_video_filter);
		nemoshow_one_set_userdata(canvas, tile);
		nemoshow_attach_one(show, canvas);

		tile->decoderback = nemoplay_back_create_decoder(tile->play);
		tile->audioback = nemoplay_back_create_audio_by_ao(tile->play);
		tile->videoback = nemoplay_back_create_video_by_timer(tile->play, tool);
		nemoplay_back_set_video_texture(tile->videoback,
				nemoshow_canvas_get_texture(tile->video),
				nemoplay_get_video_width(tile->play),
				nemoplay_get_video_height(tile->play));
		nemoplay_back_set_video_update(tile->videoback, nemotile_dispatch_video_update);
		nemoplay_back_set_video_done(tile->videoback, nemotile_dispatch_video_done);
		nemoplay_back_set_video_data(tile->videoback, tile);

		nemotile_prepare_video(tile, columns, rows, padding);

		tile->is_single = 1;
	}

	if (tile->is_3d != 0) {
		nemotile_prepare_depth(tile);
		nemotile_sort_depth(tile);
	}

	if (tile->is_3d != 0 && tile->wall != NULL) {
		nemotile_prepare_wall(tile, 0.0f, is_united_wall);
	}

	if (tile->is_3d != 0 && tile->over != NULL) {
		nemotile_prepare_ceil(tile);
	}

	if (tile->is_3d != 0 && tile->test != NULL) {
		nemotile_prepare_depthtest(tile, 5, padding);
	}

	tile->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemotile_dispatch_timer);
	nemotimer_set_userdata(timer, tile);
	nemotimer_set_timeout(timer, tile->timeout);

	nemoshow_set_keyboard_focus(show, tile->canvas);

	if (fullscreen == NULL)
		nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemotimer_destroy(tile->timer);

	nemotile_finish_tiles(tile);
	nemotile_finish_opengl(tile);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	nemotrans_group_destroy(tile->trans_group);

	free(tile);

err1:
	return 0;
}
