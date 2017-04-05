#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoyoyo.h>
#include <yoyoone.h>
#include <yoyodotor.h>
#include <nemojson.h>
#include <nemofs.h>
#include <nemomisc.h>

static int nemoyoyo_load_json(struct nemoyoyo *yoyo, const char *jsonpath)
{
	struct nemojson *json;
	const char *spoturl;

	json = nemojson_create_file(jsonpath);
	if (json == NULL)
		return -1;
	nemojson_update(json);

	spoturl = nemojson_search_string(json, 0, NULL, 2, "spot", "url");
	if (spoturl != NULL) {
		struct cooktex *tex;
		struct fsdir *contents;
		int i;

		contents = nemofs_dir_create(128);
		nemofs_dir_scan_extensions(contents, spoturl, 3, "png", "jpg", "jpeg");

		yoyo->spot.textures = (struct cooktex **)malloc(sizeof(struct cooktex *) * nemofs_dir_get_filecount(contents));
		yoyo->spot.ntextures = nemofs_dir_get_filecount(contents);

		for (i = 0; i < nemofs_dir_get_filecount(contents); i++) {
			tex = yoyo->spot.textures[i] = nemocook_texture_create();
			nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, 0, 0);
			nemocook_texture_load_image(tex, nemofs_dir_get_filepath(contents, i));
		}
	}

	nemojson_destroy(json);

	return 0;
}

static int nemoyoyo_update_one(struct nemoyoyo *yoyo)
{
	struct yoyoone *one;

	nemolist_for_each(one, &yoyo->one_list, link) {
		if (nemoyoyo_one_has_no_dirty(one) == 0)
			nemoyoyo_one_update(yoyo, one);
	}

	return 0;
}

static int nemoyoyo_update_frame(struct nemoyoyo *yoyo)
{
	struct yoyoone *one;
	pixman_region32_t damage;

	pixman_region32_init(&damage);

	nemocook_egl_fetch_damage(yoyo->egl, &damage);
	nemocook_egl_rotate_damage(yoyo->egl, &yoyo->damage);

	pixman_region32_union(&damage, &damage, &yoyo->damage);

	nemocook_egl_make_current(yoyo->egl);
	nemocook_egl_update_state(yoyo->egl);

	nemocook_egl_use_shader(yoyo->egl, yoyo->shader);

	nemolist_for_each(one, &yoyo->one_list, link) {
		pixman_region32_t region;

		pixman_region32_init(&region);
		pixman_region32_intersect(&region, &one->bounds, &damage);

		if (pixman_region32_not_empty(&region)) {
			float vertices[64 * 8];
			float texcoords[64 * 8];
			int slices[64];
			int count;

			nemocook_shader_set_uniform_matrix4fv(yoyo->shader, 0, nemocook_polygon_get_matrix4fv(one->poly));
			nemocook_shader_set_uniform_1f(yoyo->shader, 1, one->alpha);

			nemocook_polygon_update_state(one->poly);

			nemocook_texture_update_state(one->tex);
			nemocook_texture_bind(one->tex);

			count = nemoyoyo_one_clip_slice(one, &region, vertices, texcoords, slices);

			nemocook_shader_set_attrib_pointer(yoyo->shader, 0, vertices);
			nemocook_shader_set_attrib_pointer(yoyo->shader, 1, texcoords);

			nemocook_draw_arrays(GL_TRIANGLE_FAN, count, slices);

			nemocook_shader_put_attrib_pointer(yoyo->shader, 0);
			nemocook_shader_put_attrib_pointer(yoyo->shader, 1);

			nemocook_texture_unbind(one->tex);
		}

		pixman_region32_fini(&region);
	}

	nemocook_egl_swap_buffers_with_damage(yoyo->egl, &yoyo->damage);

	pixman_region32_fini(&damage);

	return 0;
}

static int nemoyoyo_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemoaction_get_userdata(action);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		struct yoyodotor *dotor;

		dotor = nemoyoyo_dotor_create(yoyo, tap);
		nemoyoyo_dotor_set_minimum_interval(dotor, 12);
		nemoyoyo_dotor_set_maximum_interval(dotor, 24);
		nemoyoyo_dotor_dispatch(dotor, yoyo->tool);
	}

	return 0;
}

static void nemoyoyo_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemocanvas_get_userdata(canvas);

	yoyo->width = width;
	yoyo->height = height;

	nemocanvas_egl_resize(yoyo->canvas, width, height);
}

static void nemoyoyo_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemocanvas_get_userdata(canvas);

	nemotransition_group_dispatch(yoyo->transitions, time_current_msecs());

	nemoyoyo_update_one(yoyo);
	nemoyoyo_update_frame(yoyo);
}

static int nemoyoyo_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemocanvas_get_userdata(canvas);
	struct actiontap *tap;

	if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		tap = nemoaction_tap_create(yoyo->action);
		nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
		nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
		nemoaction_tap_set_device(tap, nemoevent_get_device(event));
		nemoaction_tap_set_serial(tap, nemoevent_get_serial(event));
		nemoaction_tap_dispatch_event(yoyo->action, tap, NEMOACTION_TAP_DOWN_EVENT);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		tap = nemoaction_get_tap_by_device(yoyo->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_detach(tap);
			nemoaction_tap_dispatch_event(yoyo->action, tap, NEMOACTION_TAP_UP_EVENT);
			nemoaction_tap_destroy(tap);
		}
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		tap = nemoaction_get_tap_by_device(yoyo->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_dispatch_event(yoyo->action, tap, NEMOACTION_TAP_MOTION_EVENT);
		}
	}

	return 0;
}

static int nemoyoyo_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	nemotool_exit(nemocanvas_get_tool(canvas));

	return 1;
}

int main(int argc, char *argv[])
{
	static const char *vertexshader_texture =
		"uniform mat4 transform;\n"
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = transform * vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";
	static const char *fragmentshader_texture =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"uniform float alpha;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord);\n"
		"  gl_FragColor.a = gl_FragColor.a * alpha;\n"
		"}\n";

	struct option options[] = {
		{ "width",						required_argument,		NULL,		'w' },
		{ "height",						required_argument,		NULL,		'h' },
		{ "fullscreen",				required_argument,		NULL,		'f' },
		{ "config",						required_argument,		NULL,		'c' },
		{ 0 }
	};

	struct nemoyoyo *yoyo;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct cookegl *egl;
	struct cookshader *shader;
	struct cooktrans *trans;
	struct nemoaction *action;
	char *configpath = NULL;
	char *fullscreenid = NULL;
	int width = 1920;
	int height = 1080;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:c:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'f':
				fullscreenid = strdup(optarg);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (configpath == NULL)
		return 0;

	yoyo = (struct nemoyoyo *)malloc(sizeof(struct nemoyoyo));
	if (yoyo == NULL)
		return -1;
	memset(yoyo, 0, sizeof(struct nemoyoyo));

	pixman_region32_init(&yoyo->damage);

	nemolist_init(&yoyo->one_list);

	yoyo->transitions = nemotransition_group_create();

	tool = yoyo->tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);
	nemotool_connect_egl(tool);

	canvas = yoyo->canvas = nemocanvas_egl_create(tool, width, height);
	nemocanvas_set_nemosurface(canvas, "normal");
	nemocanvas_set_dispatch_resize(canvas, nemoyoyo_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemoyoyo_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemoyoyo_dispatch_canvas_event);
	nemocanvas_set_dispatch_destroy(canvas, nemoyoyo_dispatch_canvas_destroy);
	nemocanvas_set_userdata(canvas, yoyo);

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	egl = yoyo->egl = nemocook_egl_create(
			NTEGL_DISPLAY(tool),
			NTEGL_CONTEXT(tool),
			NTEGL_CONFIG(tool),
			NTEGL_WINDOW(canvas));
	nemocook_egl_resize(egl, width, height);

	nemocook_egl_attach_state(egl,
			nemocook_state_create(1, NEMOCOOK_STATE_COLOR_BUFFER_TYPE, 0.0f, 0.0f, 0.0f, 0.0f));

	shader = yoyo->shader = nemocook_shader_create();
	nemocook_shader_set_program(shader, vertexshader_texture, fragmentshader_texture);
	nemocook_shader_set_attrib(shader, 0, "position", 2);
	nemocook_shader_set_attrib(shader, 1, "texcoord", 2);
	nemocook_shader_set_uniform(shader, 0, "transform");
	nemocook_shader_set_uniform(shader, 1, "alpha");

	trans = yoyo->projection = nemocook_transform_create();
	nemocook_transform_set_translate(trans, -1.0f, 1.0f, 0.0f);
	nemocook_transform_set_scale(trans, 2.0f / (float)width, -2.0f / (float)height, 1.0f);
	nemocook_transform_set_state(trans, NEMOCOOK_TRANSFORM_NOPIN_STATE);
	nemocook_transform_update(trans);

	action = yoyo->action = nemoaction_create();
	nemoaction_set_tap_callback(action, nemoyoyo_dispatch_tap_event);
	nemoaction_set_userdata(action, yoyo);

	nemoyoyo_load_json(yoyo, configpath);

	nemocanvas_dispatch_frame(canvas);

	nemotool_run(tool);

	nemoaction_destroy(action);

	nemocook_transform_destroy(trans);
	nemocook_shader_destroy(shader);
	nemocook_egl_destroy(egl);

	nemocanvas_egl_destroy(canvas);

	nemotool_destroy(tool);

	nemotransition_group_destroy(yoyo->transitions);

	pixman_region32_fini(&yoyo->damage);

	free(yoyo);

	return 0;
}

void nemoyoyo_attach_one(struct nemoyoyo *yoyo, struct yoyoone *one)
{
	nemolist_insert_tail(&yoyo->one_list, &one->link);

	nemocook_transform_set_parent(one->trans, yoyo->projection);
}

void nemoyoyo_detach_one(struct nemoyoyo *yoyo, struct yoyoone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);

	nemocook_transform_set_parent(one->trans, NULL);
}
