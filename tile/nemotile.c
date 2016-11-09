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

static void nemotile_one_set_vertex(struct tileone *one, int index, float x, float y)
{
	one->vertices[index * 2 + 0] = x;
	one->vertices[index * 2 + 1] = y;
}

static void nemotile_one_translate_vertices(struct tileone *one, float tx, float ty)
{
	one->vtransform.tx = tx;
	one->vtransform.ty = ty;
}

static void nemotile_one_rotate_vertices(struct tileone *one, float r)
{
	one->vtransform.r = r;
}

static void nemotile_one_scale_vertices(struct tileone *one, float sx, float sy)
{
	one->vtransform.sx = sx;
	one->vtransform.sy = sy;
}

static void nemotile_one_set_texcoord(struct tileone *one, int index, float tx, float ty)
{
	one->texcoords[index * 2 + 0] = tx;
	one->texcoords[index * 2 + 1] = ty;
}

static void nemotile_one_translate_texcoords(struct tileone *one, float tx, float ty)
{
	one->ttransform.tx = tx;
	one->ttransform.ty = ty;
}

static void nemotile_one_rotate_texcoords(struct tileone *one, float r)
{
	one->ttransform.r = r;
}

static void nemotile_one_scale_texcoords(struct tileone *one, float sx, float sy)
{
	one->ttransform.sx = sx;
	one->ttransform.sy = sy;
}

static void nemotile_one_set_texture(struct tileone *one, struct showone *canvas)
{
	one->texture = canvas;
}

static void nemotile_dispatch_canvas_redraw(struct nemoshow *show, struct showone *canvas)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);
	struct tileone *one;
	struct nemomatrix vtransform, ttransform;
	uint32_t msecs = time_current_msecs();
	float dt;

	if (tile->msecs == 0)
		tile->msecs = msecs;

	dt = (float)(msecs - tile->msecs) / 1000.0f;

	glBindFramebuffer(GL_FRAMEBUFFER, tile->fbo);

	glViewport(0, 0,
			nemoshow_canvas_get_viewport_width(canvas),
			nemoshow_canvas_get_viewport_height(canvas));

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(tile->program);
	glBindAttribLocation(tile->program, 0, "position");
	glBindAttribLocation(tile->program, 1, "texcoord");

	glUniform1i(tile->utexture, 0);

	nemolist_for_each(one, &tile->tile_list, link) {
		nemomatrix_init_identity(&vtransform);
		nemomatrix_rotate(&vtransform, cos(one->vtransform.r), sin(one->vtransform.r));
		nemomatrix_scale(&vtransform, one->vtransform.sx, one->vtransform.sy);
		nemomatrix_translate(&vtransform, one->vtransform.tx, one->vtransform.ty);

		nemomatrix_init_identity(&ttransform);
		nemomatrix_rotate(&ttransform, cos(one->ttransform.r), sin(one->ttransform.r));
		nemomatrix_scale(&ttransform, one->ttransform.sx, one->ttransform.sy);
		nemomatrix_translate(&ttransform, one->ttransform.tx, one->ttransform.ty);

		glUniformMatrix4fv(tile->uvtransform, 1, GL_FALSE, vtransform.d);
		glUniformMatrix4fv(tile->uttransform, 1, GL_FALSE, ttransform.d);

		glBindTexture(GL_TEXTURE_2D, nemoshow_canvas_get_texture(one->texture));

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->vertices[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), &one->texcoords[0]);
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, one->count);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	nemoshow_one_dirty(canvas, NEMOSHOW_REDRAW_DIRTY);
}

static void nemotile_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemotile *tile = (struct nemotile *)nemoshow_get_userdata(show);

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
	int width = nemoshow_canvas_get_viewport_width(tile->canvas);
	int height = nemoshow_canvas_get_viewport_height(tile->canvas);

	nemoshow_one_dirty(tile->canvas, NEMOSHOW_REDRAW_DIRTY);
	nemoshow_dispatch_frame(tile->show);

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
		"varying vec4 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord.xy);\n"
		"}\n";

	fbo_prepare_context(
			nemoshow_canvas_get_texture(tile->canvas),
			width, height,
			&tile->fbo, &tile->dbo);

	tile->program = glshader_compile_program(vertexshader, fragmentshader, NULL, NULL);

	tile->uvtransform = glGetUniformLocation(tile->program, "vtransform");
	tile->uttransform = glGetUniformLocation(tile->program, "ttransform");
	tile->utexture = glGetUniformLocation(tile->program, "texture");

	return 0;
}

static void nemotile_finish_opengl(struct nemotile *tile)
{
	glDeleteFramebuffers(1, &tile->fbo);
	glDeleteRenderbuffers(1, &tile->dbo);

	glDeleteProgram(tile->program);
}

static int nemotile_prepare_tiles(struct nemotile *tile, int columns, int rows)
{
	struct tileone *one;
	float dx = columns % 2 == 0 ? 0.5f : 0.0f;
	float dy = rows % 2 == 0 ? 0.5f : 0.0f;
	int x, y;

	for (y = 0; y < rows; y++) {
		for (x = 0; x < columns; x++) {
			one = nemotile_one_create(5);
			nemotile_one_set_vertex(one, 0, -1.0f, 1.0f);
			nemotile_one_set_texcoord(one, 0, 0.0f, 1.0f);
			nemotile_one_set_vertex(one, 1, 1.0f, 1.0f);
			nemotile_one_set_texcoord(one, 1, 1.0f, 1.0f);
			nemotile_one_set_vertex(one, 2, 1.0f, -1.0f);
			nemotile_one_set_texcoord(one, 2, 1.0f, 0.0f);
			nemotile_one_set_vertex(one, 3, -1.0f, -1.0f);
			nemotile_one_set_texcoord(one, 3, 0.0f, 0.0f);
			nemotile_one_set_vertex(one, 4, -1.0f, 1.0f);
			nemotile_one_set_texcoord(one, 4, 0.0f, 1.0f);

			nemotile_one_set_texture(one, tile->video);

			nemotile_one_translate_vertices(one,
					2.0f / (float)columns * (x - columns / 2 + dx),
					2.0f / (float)rows * (rows / 2 - y - dy));
			nemotile_one_scale_vertices(one,
					1.0f / (float)columns,
					1.0f / (float)rows);

			nemotile_one_translate_texcoords(one,
					1.0f / (float)columns * x,
					1.0f / (float)rows * (rows - y - 1));
			nemotile_one_scale_texcoords(one,
					1.0f / (float)columns,
					1.0f / (float)rows);

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
	int timeout = 10000;
	int width = 800;
	int height = 800;
	int columns = 1;
	int rows = 1;
	int fps = 60;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:c:r:i:v:t:b:o:f:m:", options, NULL)) {
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
	tile->timeout = timeout;

	nemolist_init(&tile->tile_list);

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
	nemotile_prepare_tiles(tile, columns, rows);

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
