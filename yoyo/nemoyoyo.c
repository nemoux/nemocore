#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoyoyo.h>
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
		int width = nemojson_search_integer(json, 0, 128, 2, "spot", "width");
		int height = nemojson_search_integer(json, 0, 128, 2, "spot", "height");
		int i;

		contents = nemofs_dir_create(128);
		nemofs_dir_scan_extensions(contents, spoturl, 3, "png", "jpg", "jpeg");

		yoyo->spot.textures = (struct cooktex **)malloc(sizeof(struct cooktex *) * nemofs_dir_get_filecount(contents));
		yoyo->spot.ntextures = nemofs_dir_get_filecount(contents);

		for (i = 0; i < nemofs_dir_get_filecount(contents); i++) {
			tex = yoyo->spot.textures[i] = nemocook_texture_create();
			nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, width, height);
			nemocook_texture_load_image(tex, nemofs_dir_get_filepath(contents, i));
		}
	}

	nemojson_destroy(json);

	return 0;
}

static int nemoyoyo_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemoaction_get_userdata(action);

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
	pixman_region32_t damage;

	pixman_region32_init(&damage);

	nemocook_egl_fetch_damage(yoyo->egl, &damage);
	nemocook_egl_rotate_damage(yoyo->egl, &yoyo->damage);

	pixman_region32_union(&damage, &damage, &yoyo->damage);

	nemocook_egl_make_current(yoyo->egl);
	nemocook_egl_update_state(yoyo->egl);

	nemocook_egl_use_shader(yoyo->egl, yoyo->shader);

	nemocook_egl_swap_buffers_with_damage(yoyo->egl, &yoyo->damage);

	pixman_region32_fini(&damage);
}

static int nemoyoyo_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemocanvas_get_userdata(canvas);

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

	nemolist_init(&yoyo->spot_list);

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

	action = yoyo->action = nemoaction_create();
	nemoaction_set_tap_callback(action, nemoyoyo_dispatch_tap_event);
	nemoaction_set_userdata(action, yoyo);

	nemoyoyo_load_json(yoyo, configpath);

	nemocanvas_dispatch_frame(canvas);

	nemotool_run(tool);

	nemoaction_destroy(action);

	nemocook_shader_destroy(shader);
	nemocook_egl_destroy(egl);

	nemocanvas_egl_destroy(canvas);

	nemotool_destroy(tool);

	pixman_region32_fini(&yoyo->damage);

	free(yoyo);

	return 0;
}
