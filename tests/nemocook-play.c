#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemocook.h>
#include <nemoplay.h>
#include <playback.h>
#include <nemomisc.h>

struct cookcontext {
	struct nemotool *tool;
	struct nemoegl *egl;
	struct nemocanvas *canvas;

	struct nemocook *cook;
	struct cookshader *shader;
	struct cooktex *backtex;
	struct cooktex *videotex;
	struct cookpoly *backpoly;
	struct cookpoly *videopoly;

	struct nemoplay *play;
	struct playdecoder *decoderback;
	struct playaudio *audioback;
	struct playvideo *videoback;

	int width, height;
};

static void nemocook_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct cookcontext *context = (struct cookcontext *)nemocanvas_get_userdata(canvas);
}

static void nemocook_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct cookcontext *context = (struct cookcontext *)nemocanvas_get_userdata(canvas);

	nemocook_render(context->cook);
}

static int nemocook_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct cookcontext *context = (struct cookcontext *)nemocanvas_get_userdata(canvas);

	return 0;
}

static void nemocook_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct cookcontext *context = (struct cookcontext *)data;

	nemocanvas_dispatch_frame(context->canvas);
}

static void nemocook_dispatch_video_done(struct nemoplay *play, void *data)
{
	struct cookcontext *context = (struct cookcontext *)data;

	nemoplay_decoder_seek(context->decoderback, 0.0f);
}

int main(int argc, char *argv[])
{
	static const char *vertexshader =
		"uniform mat4 transform;\n"
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = transform * vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";
	static const char *fragmentshader =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord);\n"
		"}\n";

	struct option options[] = {
		{ "width",			required_argument,		NULL,		'w' },
		{ "height",			required_argument,		NULL,		'h' },
		{ "fullscreen",	required_argument,		NULL,		'f' },
		{ 0 }
	};

	struct cookcontext *context;
	struct nemotool *tool;
	struct nemoegl *egl;
	struct nemocanvas *canvas;
	struct nemocook *cook;
	struct cookshader *shader;
	struct cooktex *tex;
	struct cookpoly *poly;
	struct nemoplay *play;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	int width = 1280;
	int height = 720;
	int opt;

	while (opt = getopt_long(argc, argv, "w:h:f:", options, NULL)) {
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

			default:
				break;
		}
	}

	if (optind < argc)
		contentpath = strdup(argv[optind]);

	if (contentpath == NULL)
		return 0;

	context = (struct cookcontext *)malloc(sizeof(struct cookcontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct cookcontext));

	context->width = width;
	context->height = height;

	context->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);

	context->egl = egl = nemoegl_create(tool);
	context->canvas = canvas = nemoegl_create_canvas(egl, width, height);
	nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(canvas, nemocook_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemocook_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemocook_dispatch_canvas_event);
	nemocanvas_set_userdata(canvas, context);

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	context->cook = cook = nemocook_create();
	nemocook_set_size(cook, width, height);
	nemocook_egl_prepare(cook,
			NTEGL_DISPLAY(egl),
			NTEGL_CONTEXT(egl),
			NTEGL_CONFIG(egl),
			NTEGL_WINDOW(canvas));
	nemocook_renderer_prepare(cook);

	context->shader = shader = nemocook_shader_create();
	nemocook_shader_set_program(shader, vertexshader, fragmentshader);
	nemocook_shader_set_attrib(shader, 0, "position", 2);
	nemocook_shader_set_attrib(shader, 1, "texcoord", 2);

	context->backtex = tex = nemocook_texture_create();
	nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, width, height);
	context->backpoly = poly = nemocook_polygon_create();
	nemocook_polygon_set_texture(poly, tex);
	nemocook_polygon_set_shader(poly, shader);
	nemocook_polygon_set_type(poly, GL_TRIANGLE_STRIP);
	nemocook_polygon_set_count(poly, 4);
	nemocook_polygon_set_buffer(poly, 0, 2);
	nemocook_polygon_set_buffer(poly, 1, 2);
	nemocook_polygon_set_element(poly, 0, 0, 0, -1.0f);
	nemocook_polygon_set_element(poly, 0, 0, 1, 1.0f);
	nemocook_polygon_set_element(poly, 0, 1, 0, 1.0f);
	nemocook_polygon_set_element(poly, 0, 1, 1, 1.0f);
	nemocook_polygon_set_element(poly, 0, 2, 0, -1.0f);
	nemocook_polygon_set_element(poly, 0, 2, 1, -1.0f);
	nemocook_polygon_set_element(poly, 0, 3, 0, 1.0f);
	nemocook_polygon_set_element(poly, 0, 3, 1, -1.0f);
	nemocook_polygon_set_element(poly, 1, 0, 0, 0.0f);
	nemocook_polygon_set_element(poly, 1, 0, 1, 0.0f);
	nemocook_polygon_set_element(poly, 1, 1, 0, 1.0f);
	nemocook_polygon_set_element(poly, 1, 1, 1, 0.0f);
	nemocook_polygon_set_element(poly, 1, 2, 0, 0.0f);
	nemocook_polygon_set_element(poly, 1, 2, 1, 1.0f);
	nemocook_polygon_set_element(poly, 1, 3, 0, 1.0f);
	nemocook_polygon_set_element(poly, 1, 3, 1, 1.0f);
	nemocook_attach_polygon(cook, poly);

	context->play = play = nemoplay_create();
	nemoplay_load_media(play, contentpath);

	context->decoderback = nemoplay_decoder_create(play);
	context->audioback = nemoplay_audio_create_by_ao(play);
	context->videoback = nemoplay_video_create_by_timer(play, tool);
	nemoplay_video_set_texture(context->videoback,
			nemocook_texture_get(context->backtex),
			width, height);
	nemoplay_video_set_update(context->videoback, nemocook_dispatch_video_update);
	nemoplay_video_set_done(context->videoback, nemocook_dispatch_video_done);
	nemoplay_video_set_data(context->videoback, context);

	nemocanvas_dispatch_frame(canvas);

	nemotool_run(tool);

	nemocook_shader_destroy(shader);
	nemocook_destroy(cook);

	nemoegl_destroy_canvas(canvas);

	nemoegl_destroy(egl);

	nemotool_destroy(tool);

	free(context);

	return 0;
}
