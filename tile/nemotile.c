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
#include <fbohelper.h>
#include <glshader.h>
#include <nemofs.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

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

static struct tileone *nemotile_one_create(int vertices)
{
	struct tileone *one;

	one = (struct tileone *)malloc(sizeof(struct tileone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct tileone));

	one->vertices = (float *)malloc(sizeof(float[2]) * vertices);
	one->texcoords = (float *)malloc(sizeof(float[2]) * vertices);

	one->count = vertices;

	one->vtransform0.tx = 0.0f;
	one->vtransform0.ty = 0.0f;
	one->vtransform0.r = 0.0f;
	one->vtransform0.sx = 1.0f;
	one->vtransform0.sy = 1.0f;

	one->ttransform0.tx = 0.0f;
	one->ttransform0.ty = 0.0f;
	one->ttransform0.r = 0.0f;
	one->ttransform0.sx = 1.0f;
	one->ttransform0.sy = 1.0f;

	one->vtransform.tx = 0.0f;
	one->vtransform.ty = 0.0f;
	one->vtransform.r = 0.0f;
	one->vtransform.sx = 1.0f;
	one->vtransform.sy = 1.0f;

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

	free(one);
}

static void nemotile_one_set_index(struct tileone *one, int index)
{
	one->index = index;
}

static void nemotile_one_set_vertex(struct tileone *one, int index, float x, float y)
{
	one->vertices[index * 2 + 0] = x;
	one->vertices[index * 2 + 1] = y;
}

static void nemotile_one_vertices_translate(struct tileone *one, float tx, float ty)
{
	one->vtransform.tx = tx;
	one->vtransform.ty = ty;
}

static void nemotile_one_vertices_rotate(struct tileone *one, float r)
{
	one->vtransform.r = r;
}

static void nemotile_one_vertices_scale(struct tileone *one, float sx, float sy)
{
	one->vtransform.sx = sx;
	one->vtransform.sy = sy;
}

static void nemotile_one_vertices_translate_to(struct tileone *one, float tx, float ty)
{
	one->vtransform0.tx = tx;
	one->vtransform0.ty = ty;
}

static void nemotile_one_vertices_rotate_to(struct tileone *one, float r)
{
	one->vtransform0.r = r;
}

static void nemotile_one_vertices_scale_to(struct tileone *one, float sx, float sy)
{
	one->vtransform0.sx = sx;
	one->vtransform0.sy = sy;
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

static void nemotile_one_set_color(struct tileone *one, float r, float g, float b, float a)
{
	one->color[0] = r;
	one->color[1] = g;
	one->color[2] = b;
	one->color[3] = a;
}

static struct tileone *nemotile_pick_one(struct nemotile *tile, float x, float y)
{
	struct tileone *one;
	struct nemomatrix matrix, inverse;

	nemolist_for_each_reverse(one, &tile->tile_list, link) {
		nemomatrix_init_identity(&matrix);
		nemomatrix_rotate(&matrix, cos(one->vtransform.r), sin(one->vtransform.r));
		nemomatrix_scale(&matrix, one->vtransform.sx, one->vtransform.sy);
		nemomatrix_translate(&matrix, one->vtransform.tx, one->vtransform.ty);

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

static struct tileone *nemotile_find_one(struct nemotile *tile, int index)
{
	struct tileone *one;

	nemolist_for_each(one, &tile->tile_list, link) {
		if (one->index == index)
			return one;
	}

	return NULL;
}

static void nemotile_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);
	struct tileone *one;
	struct nemotrans *trans, *ntrans;
	struct nemomatrix vtransform, ttransform;
	uint32_t msecs = time_current_msecs();
	float linecolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	nemolist_for_each_safe(trans, ntrans, &tile->trans_list, link) {
		if (nemotrans_dispatch(trans, msecs) != 0)
			nemotrans_destroy(trans);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, tile->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	nemolist_for_each(one, &tile->tile_list, link) {
		nemomatrix_init_identity(&vtransform);
		nemomatrix_rotate(&vtransform, cos(one->vtransform.r), sin(one->vtransform.r));
		nemomatrix_scale(&vtransform, one->vtransform.sx, one->vtransform.sy);
		nemomatrix_translate(&vtransform, one->vtransform.tx, one->vtransform.ty);

		nemomatrix_init_identity(&ttransform);
		nemomatrix_rotate(&ttransform, cos(one->ttransform.r), sin(one->ttransform.r));
		nemomatrix_scale(&ttransform, one->ttransform.sx, one->ttransform.sy);
		nemomatrix_translate(&ttransform, one->ttransform.tx, one->ttransform.ty);

		glUseProgram(tile->programs[0]);
		glBindAttribLocation(tile->programs[0], 0, "position");
		glBindAttribLocation(tile->programs[0], 1, "texcoord");

		glUniform1i(tile->utexture0, 0);
		glUniformMatrix4fv(tile->uvtransform0, 1, GL_FALSE, vtransform.d);
		glUniformMatrix4fv(tile->uttransform0, 1, GL_FALSE, ttransform.d);
		glUniform4fv(tile->ucolor0, 1, one->color);

		glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(one->texture));

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->texcoords[0]);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, one->count);

		glBindTexture(GL_TEXTURE_2D, 0);

		if (tile->linewidth > 0.0f) {
			glUseProgram(tile->programs[1]);
			glBindAttribLocation(tile->programs[1], 0, "position");

			glUniformMatrix4fv(tile->uvtransform1, 1, GL_FALSE, vtransform.d);
			glUniform4fv(tile->ucolor1, 1, linecolor);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->vertices[0]);
			glEnableVertexAttribArray(0);

			glLineWidth(tile->linewidth);
			glDrawArrays(GL_LINE_STRIP, 0, one->count);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
}

static void nemotile_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);
	struct tileone *one;
	struct nemotrans *trans;

	if (nemoshow_event_is_pointer_left_down(show, event)) {
		nemolist_for_each(one, &tile->tile_list, link) {
			trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
					random_get_int(1200, 2400),
					random_get_int(100, 300));

			nemotrans_set_float(trans, &one->ttransform.tx, 0.0f);
			nemotrans_set_float(trans, &one->ttransform.ty, 0.0f);
			nemotrans_set_float(trans, &one->ttransform.sx, 1.0f);
			nemotrans_set_float(trans, &one->ttransform.sy, 1.0f);

			nemolist_insert_tail(&tile->trans_list, &trans->link);
		}

		tile->state = 1;
	} else if (nemoshow_event_is_pointer_left_up(show, event)) {
		nemolist_for_each(one, &tile->tile_list, link) {
			trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
					random_get_int(1200, 2400),
					random_get_int(100, 300));

			nemotrans_set_float(trans, &one->ttransform.tx, one->ttransform0.tx);
			nemotrans_set_float(trans, &one->ttransform.ty, one->ttransform0.ty);
			nemotrans_set_float(trans, &one->ttransform.sx, one->ttransform0.sx);
			nemotrans_set_float(trans, &one->ttransform.sy, one->ttransform0.sy);

			nemolist_insert_tail(&tile->trans_list, &trans->link);
		}

		tile->state = 0;
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

			nemolist_insert_tail(&tile->trans_list, &trans->link);
		}

		tile->state = 1;
	} else if (nemoshow_event_is_pointer_right_up(show, event)) {
		nemolist_for_each(one, &tile->tile_list, link) {
			trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
					random_get_int(1200, 1800),
					random_get_int(100, 300));

			nemotrans_set_float(trans, &one->color[0], random_get_double(tile->brightness, 1.0f));
			nemotrans_set_float(trans, &one->color[1], random_get_double(tile->brightness, 1.0f));
			nemotrans_set_float(trans, &one->color[2], random_get_double(tile->brightness, 1.0f));
			nemotrans_set_float(trans, &one->color[3], random_get_double(tile->brightness, 1.0f));

			nemolist_insert_tail(&tile->trans_list, &trans->link);
		}

		tile->state = 0;
	}

	if (nemoshow_event_is_touch_down(show, event)) {
		struct tileone *pone;

		pone = nemotile_pick_one(tile,
				(nemoshow_event_get_x(event) / tile->width * 2.0f) - 1.0f,
				(nemoshow_event_get_y(event) / tile->height * 2.0f) - 1.0f);
		if (pone != NULL) {
			nemolist_remove(&pone->link);
			nemolist_insert_tail(&tile->tile_list, &pone->link);

			nemolist_for_each(one, &tile->tile_list, link) {
				trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
						random_get_int(600, 1200),
						random_get_int(30, 300));

				nemotrans_set_float(trans, &one->ttransform.tx, 0.0f);
				nemotrans_set_float(trans, &one->ttransform.ty, 0.0f);
				nemotrans_set_float(trans, &one->ttransform.sx, 1.0f);
				nemotrans_set_float(trans, &one->ttransform.sy, 1.0f);
				nemotrans_set_float(trans, &one->vtransform.tx, pone->vtransform.tx);
				nemotrans_set_float(trans, &one->vtransform.ty, pone->vtransform.ty);

				nemolist_insert_tail(&tile->trans_list, &trans->link);
			}

			tile->state = 1;
		}
	} else if (nemoshow_event_is_touch_up(show, event)) {
		nemolist_for_each(one, &tile->tile_list, link) {
			trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
					random_get_int(600, 1200),
					random_get_int(30, 300));

			nemotrans_set_float(trans, &one->ttransform.tx, one->ttransform0.tx);
			nemotrans_set_float(trans, &one->ttransform.ty, one->ttransform0.ty);
			nemotrans_set_float(trans, &one->ttransform.sx, one->ttransform0.sx);
			nemotrans_set_float(trans, &one->ttransform.sy, one->ttransform0.sy);
			nemotrans_set_float(trans, &one->vtransform.tx, one->vtransform0.tx);
			nemotrans_set_float(trans, &one->vtransform.ty, one->vtransform0.ty);

			nemolist_insert_tail(&tile->trans_list, &trans->link);
		}

		tile->state = 0;
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

static void nemotile_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);

	nemoshow_view_resize(show, width, height);

	glDeleteFramebuffers(1, &tile->fbo);
	glDeleteRenderbuffers(1, &tile->dbo);

	fbo_prepare_context(
			nemoshow_canvas_get_texture(tile->canvas),
			width, height,
			&tile->fbo, &tile->dbo);

	nemoshow_one_dirty(tile->canvas, NEMOSHOW_REDRAW_DIRTY);

	nemoshow_view_redraw(show);
}

static void nemotile_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;

	if (tile->state == 0) {
		struct nemotrans *trans;
		struct tileone *one;

		if (tile->iactions == 0) {
			nemolist_for_each(one, &tile->tile_list, link) {
				trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
						random_get_int(700, 1400),
						random_get_int(100, 500));

				nemotrans_set_float(trans, &one->vtransform.r, one->vtransform.r + M_PI);

				nemotrans_set_float(trans, &one->color[0], random_get_double(tile->brightness, 1.0f));
				nemotrans_set_float(trans, &one->color[1], random_get_double(tile->brightness, 1.0f));
				nemotrans_set_float(trans, &one->color[2], random_get_double(tile->brightness, 1.0f));
				nemotrans_set_float(trans, &one->color[3], random_get_double(tile->brightness, 1.0f));

				nemotile_one_texcoords_translate_to(one,
						0.5f - (one->ttransform0.tx - 0.5f) - nemotile_get_column_sx(tile->columns),
						0.5f - (one->ttransform0.ty - 0.5f) - nemotile_get_row_sy(tile->rows));

				nemotrans_set_float(trans, &one->ttransform.tx, one->ttransform0.tx);
				nemotrans_set_float(trans, &one->ttransform.ty, one->ttransform0.ty);

				nemolist_insert_tail(&tile->trans_list, &trans->link);
			}
		} else if (tile->iactions == 1) {
			struct tileone *none;
			float tx, ty;
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

					tx = one->ttransform0.tx;
					ty = one->ttransform0.ty;
					one->ttransform0.tx = none->ttransform0.tx;
					one->ttransform0.ty = none->ttransform0.ty;
					none->ttransform0.tx = tx;
					none->ttransform0.ty = ty;
				}
			}

			nemolist_for_each(one, &tile->tile_list, link) {
				trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
						random_get_int(1200, 1800),
						random_get_int(200, 400));

				nemotrans_set_float(trans, &one->vtransform.tx, one->vtransform0.tx);
				nemotrans_set_float(trans, &one->vtransform.ty, one->vtransform0.ty);

				nemolist_insert_tail(&tile->trans_list, &trans->link);

				trans = nemotrans_create(NEMOEASE_CUBIC_INOUT_TYPE,
						random_get_int(400, 800),
						random_get_int(2400, 2800));

				nemotrans_set_float(trans, &one->ttransform.tx, one->ttransform0.tx);
				nemotrans_set_float(trans, &one->ttransform.ty, one->ttransform0.ty);

				nemolist_insert_tail(&tile->trans_list, &trans->link);
			}
		}

		tile->iactions = (tile->iactions + 1) % 2;
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
	nemoplay_back_set_video_canvas(tile->videoback,
			tile->video,
			nemoplay_get_video_width(tile->play),
			nemoplay_get_video_height(tile->play));
	nemoplay_back_set_video_update(tile->videoback, nemotile_dispatch_video_update);
	nemoplay_back_set_video_done(tile->videoback, nemotile_dispatch_video_done);
	nemoplay_back_set_video_data(tile->videoback, tile);
}

static GLuint nemotile_dispatch_tale_effect(struct talenode *node, void *data)
{
	struct nemotile *tile = (struct nemotile *)data;
	GLuint texture = nemotale_node_get_texture(node);

	if (tile->motion != NULL) {
		nemofx_glmotion_dispatch(tile->motion, texture);

		texture = nemofx_glmotion_get_texture(tile->motion);
	}

	return texture;
}

static int nemotile_prepare_opengl(struct nemotile *tile, int32_t width, int32_t height)
{
	static const char *vertexshader =
		"uniform mat4 vtransform;\n"
		"uniform mat4 ttransform;\n"
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec4 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vtransform * vec4(position.xy, 0.0, 1.0);\n"
		"  vtexcoord = ttransform * vec4(texcoord.xy, 0.0, 1.0);\n"
		"}\n";
	static const char *fragmentshader =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"uniform vec4 color;\n"
		"varying vec4 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord.xy) * color;\n"
		"}\n";
	static const char *vertexshader_solid =
		"uniform mat4 vtransform;\n"
		"attribute vec2 position;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vtransform * vec4(position.xy, 0.0, 1.0);\n"
		"}\n";
	static const char *fragmentshader_solid =
		"precision mediump float;\n"
		"uniform vec4 color;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = color;\n"
		"}\n";

	fbo_prepare_context(
			nemoshow_canvas_get_texture(tile->canvas),
			width, height,
			&tile->fbo, &tile->dbo);

	tile->programs[0] = glshader_compile_program(vertexshader, fragmentshader, NULL, NULL);
	tile->programs[1] = glshader_compile_program(vertexshader_solid, fragmentshader_solid, NULL, NULL);

	tile->uvtransform0 = glGetUniformLocation(tile->programs[0], "vtransform");
	tile->uttransform0 = glGetUniformLocation(tile->programs[0], "ttransform");
	tile->utexture0 = glGetUniformLocation(tile->programs[0], "texture");
	tile->ucolor0 = glGetUniformLocation(tile->programs[0], "color");

	tile->uvtransform1 = glGetUniformLocation(tile->programs[1], "vtransform");
	tile->ucolor1 = glGetUniformLocation(tile->programs[1], "color");

	return 0;
}

static void nemotile_finish_opengl(struct nemotile *tile)
{
	glDeleteFramebuffers(1, &tile->fbo);
	glDeleteRenderbuffers(1, &tile->dbo);

	glDeleteProgram(tile->programs[0]);
	glDeleteProgram(tile->programs[1]);
}

static int nemotile_prepare_tiles(struct nemotile *tile, int columns, int rows, float padding)
{
	struct tileone *one;
	int x, y;

	for (y = 0; y < rows; y++) {
		for (x = 0; x < columns; x++) {
			one = nemotile_one_create(4);
			nemotile_one_set_index(one, (y * columns) + x);
			nemotile_one_set_vertex(one, 0, -1.0f, 1.0f);
			nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
			nemotile_one_set_vertex(one, 1, 1.0f, 1.0f);
			nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
			nemotile_one_set_vertex(one, 2, -1.0f, -1.0f);
			nemotile_one_set_texcoord(one, 2, 0.0f, 0.0f);
			nemotile_one_set_vertex(one, 3, 1.0f, -1.0f);
			nemotile_one_set_texcoord(one, 3, 1.0f, 0.0f);

			nemotile_one_set_texture(one, tile->video);

			nemotile_one_set_color(one,
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f),
					random_get_double(tile->brightness, 1.0f));

			nemotile_one_vertices_translate_to(one,
					nemotile_get_column_x(columns, x),
					nemotile_get_row_y(rows, y));
			nemotile_one_vertices_scale_to(one,
					nemotile_get_column_sx(columns) * (1.0f - padding),
					nemotile_get_row_sy(rows) * (1.0f - padding));
			nemotile_one_vertices_translate(one,
					nemotile_get_column_x(columns, x),
					nemotile_get_row_y(rows, y));
			nemotile_one_vertices_scale(one,
					nemotile_get_column_sx(columns) * (1.0f - padding),
					nemotile_get_row_sy(rows) * (1.0f - padding));

			nemotile_one_texcoords_translate_to(one,
					nemotile_get_column_tx(columns, x),
					nemotile_get_row_ty(rows, y));
			nemotile_one_texcoords_scale_to(one,
					nemotile_get_column_sx(columns),
					nemotile_get_row_sy(rows));
			nemotile_one_texcoords_translate(one,
					nemotile_get_column_tx(columns, x),
					nemotile_get_row_ty(rows, y));
			nemotile_one_texcoords_scale(one,
					nemotile_get_column_sx(columns),
					nemotile_get_row_sy(rows));

			nemolist_insert_tail(&tile->tile_list, &one->link);
		}
	}

	return 0;
}

static void nemotile_finish_tiles(struct nemotile *tile)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",					required_argument,			NULL,			'w' },
		{ "height",					required_argument,			NULL,			'h' },
		{ "columns",				required_argument,			NULL,			'c' },
		{ "rows",						required_argument,			NULL,			'r' },
		{ "image",					required_argument,			NULL,			'i' },
		{ "video",					required_argument,			NULL,			'v' },
		{ "timeout",				required_argument,			NULL,			't' },
		{ "background",			required_argument,			NULL,			'b' },
		{ "overlay",				required_argument,			NULL,			'o' },
		{ "fullscreen",			required_argument,			NULL,			'f' },
		{ "motionblur",			required_argument,			NULL,			'm' },
		{ "linewidth",			required_argument,			NULL,			'l' },
		{ "brightness",			required_argument,			NULL,			'e' },
		{ "padding",				required_argument,			NULL,			'p' },
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
	struct talenode *node;
	char *imagepath = NULL;
	char *videopath = NULL;
	char *fullscreen = NULL;
	char *background = NULL;
	char *overlay = NULL;
	float motionblur = 0.0f;
	float linewidth = 0.0f;
	float brightness = 0.85f;
	float padding = 0.0f;
	int timeout = 5000;
	int width = 800;
	int height = 800;
	int columns = 1;
	int rows = 1;
	int fps = 60;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:c:r:i:v:t:b:o:f:m:l:e:p:", options, NULL)) {
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
				background = strdup(optarg);
				break;

			case 'o':
				overlay = strdup(optarg);
				break;

			case 'f':
				fullscreen = strdup(optarg);
				break;

			case 'm':
				motionblur = strtod(optarg, NULL);
				break;

			case 'l':
				linewidth = strtod(optarg, NULL);
				break;

			case 'e':
				brightness = strtod(optarg, NULL);
				break;

			case 'p':
				padding = strtod(optarg, NULL);
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
	tile->linewidth = linewidth;
	tile->brightness = brightness;

	nemolist_init(&tile->tile_list);
	nemolist_init(&tile->trans_list);

	tile->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	tile->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_filtering_quality(show, NEMOSHOW_FILTER_HIGH_QUALITY);
	nemoshow_set_dispatch_resize(show, nemotile_dispatch_show_resize);
	nemoshow_set_userdata(show, tile);

	nemoshow_view_set_framerate(show, fps);

	if (fullscreen != NULL)
		nemoshow_view_set_fullscreen(show, fullscreen);

	tile->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	if (background != NULL) {
		tile->back = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_canvas_set_opaque(canvas, 1);
		nemoshow_one_attach(scene, canvas);

		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_uri(one, background);
	} else {
		tile->back = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
		nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 1.0f);
		nemoshow_one_attach(scene, canvas);
	}

	tile->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_redraw(canvas, nemotile_dispatch_canvas_redraw);
	nemoshow_canvas_set_dispatch_event(canvas, nemotile_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	node = nemoshow_canvas_get_node(canvas);
	nemotale_node_set_dispatch_effect(node, nemotile_dispatch_tale_effect, tile);

	if (motionblur > 0.0f) {
		tile->motion = nemofx_glmotion_create(width, height);
		nemofx_glmotion_set_step(tile->motion, motionblur);
	}

	if (overlay != NULL) {
		tile->over = canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_one_attach(scene, canvas);

		if (os_has_file_extension(overlay, "svg") != 0) {
			one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
			nemoshow_item_path_load_svg(one, overlay, 0.0f, 0.0f, width, height);
		} else {
			one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
			nemoshow_one_attach(canvas, one);
			nemoshow_item_set_x(one, 0.0f);
			nemoshow_item_set_y(one, 0.0f);
			nemoshow_item_set_width(one, width);
			nemoshow_item_set_height(one, height);
			nemoshow_item_set_uri(one, overlay);
		}
	}

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
		nemoshow_attach_one(show, canvas);

		tile->decoderback = nemoplay_back_create_decoder(tile->play);
		tile->audioback = nemoplay_back_create_audio_by_ao(tile->play);
		tile->videoback = nemoplay_back_create_video_by_timer(tile->play, tool);
		nemoplay_back_set_video_canvas(tile->videoback,
				tile->video,
				nemoplay_get_video_width(tile->play),
				nemoplay_get_video_height(tile->play));
		nemoplay_back_set_video_update(tile->videoback, nemotile_dispatch_video_update);
		nemoplay_back_set_video_done(tile->videoback, nemotile_dispatch_video_done);
		nemoplay_back_set_video_data(tile->videoback, tile);
	}

	nemotile_prepare_opengl(tile, width, height);
	nemotile_prepare_tiles(tile, columns, rows, padding);

	tile->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemotile_dispatch_timer);
	nemotimer_set_userdata(timer, tile);
	nemotimer_set_timeout(timer, tile->timeout);

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
	free(tile);

err1:
	return 0;
}
