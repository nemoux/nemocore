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
#include <pixmanhelper.h>
#include <nemomisc.h>

struct cookcontext {
	struct nemotool *tool;
	struct nemoegl *egl;
	struct nemocanvas *canvas;

	struct nemocook *cook;
	struct cookshader *shader;
	struct cooktex *backtex;
	struct cooktex *videotex;
	struct cooktex *motiontex;
	struct cookpoly *backpoly;
	struct cookpoly *videopoly;
	struct cookpoly *motionpoly;

	struct nemoplay *play;
	struct playdecoder *decoderback;
	struct playaudio *audioback;
	struct playvideo *videoback;

	struct nemoplay *motion;
	struct playextractor *extractor;
	struct playshader *player;
	struct playbox *box;
	int iframes;

	int width, height;
};

static void nemocook_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct cookcontext *context = (struct cookcontext *)nemocanvas_get_userdata(canvas);
}

static void nemocook_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct cookcontext *context = (struct cookcontext *)nemocanvas_get_userdata(canvas);
	struct playone *one;

	one = nemoplay_box_get_one(context->box, context->iframes);
	if (one != NULL) {
		nemoplay_shader_update(context->player, one);
		nemoplay_shader_dispatch(context->player);

		context->iframes = (context->iframes + 1) % nemoplay_box_get_count(context->box);
	}

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
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord);\n"
		"}\n";
	static const char *vertexshader_diffuse =
		"uniform mat4 transform;\n"
		"attribute vec2 position;\n"
		"attribute vec4 diffuse;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = transform * vec4(position, 0.0, 1.0);\n"
		"  vdiffuse = diffuse;\n"
		"}\n";
	static const char *fragmentshader_diffuse =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"varying vec4 vdiffuse;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = vdiffuse;\n"
		"}\n";

	struct option options[] = {
		{ "width",			required_argument,		NULL,		'w' },
		{ "height",			required_argument,		NULL,		'h' },
		{ "fullscreen",	required_argument,		NULL,		'f' },
		{ "image",			required_argument,		NULL,		'i' },
		{ "video",			required_argument,		NULL,		'v' },
		{ "motion",			required_argument,		NULL,		'm' },
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
	pixman_image_t *image;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	char *imagepath = NULL;
	char *videopath = NULL;
	char *motionpath = NULL;
	int width = 1280;
	int height = 720;
	int opt;

	while (opt = getopt_long(argc, argv, "w:h:f:i:v:m:", options, NULL)) {
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

			case 'i':
				imagepath = strdup(optarg);
				break;

			case 'v':
				videopath = strdup(optarg);
				break;

			case 'm':
				motionpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (optind < argc)
		contentpath = strdup(argv[optind]);

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
	nemocook_prepare_egl(cook,
			NTEGL_DISPLAY(egl),
			NTEGL_CONTEXT(egl),
			NTEGL_CONFIG(egl),
			NTEGL_WINDOW(canvas));
	nemocook_prepare_renderer(cook);

	context->shader = shader = nemocook_shader_create();
	nemocook_shader_set_program(shader, vertexshader_texture, fragmentshader_texture);
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

	context->videotex = tex = nemocook_texture_create();
	nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, width, height);
	context->videopoly = poly = nemocook_polygon_create();
	nemocook_polygon_set_texture(poly, tex);
	nemocook_polygon_set_shader(poly, shader);
	nemocook_polygon_set_type(poly, GL_TRIANGLE_STRIP);
	nemocook_polygon_set_count(poly, 4);
	nemocook_polygon_set_buffer(poly, 0, 2);
	nemocook_polygon_set_buffer(poly, 1, 2);
	nemocook_polygon_set_element(poly, 0, 0, 0, -0.75f);
	nemocook_polygon_set_element(poly, 0, 0, 1, 0.75f);
	nemocook_polygon_set_element(poly, 0, 1, 0, 0.75f);
	nemocook_polygon_set_element(poly, 0, 1, 1, 0.75f);
	nemocook_polygon_set_element(poly, 0, 2, 0, -0.75f);
	nemocook_polygon_set_element(poly, 0, 2, 1, -0.75f);
	nemocook_polygon_set_element(poly, 0, 3, 0, 0.75f);
	nemocook_polygon_set_element(poly, 0, 3, 1, -0.75f);
	nemocook_polygon_set_element(poly, 1, 0, 0, 0.0f);
	nemocook_polygon_set_element(poly, 1, 0, 1, 0.0f);
	nemocook_polygon_set_element(poly, 1, 1, 0, 1.0f);
	nemocook_polygon_set_element(poly, 1, 1, 1, 0.0f);
	nemocook_polygon_set_element(poly, 1, 2, 0, 0.0f);
	nemocook_polygon_set_element(poly, 1, 2, 1, 1.0f);
	nemocook_polygon_set_element(poly, 1, 3, 0, 1.0f);
	nemocook_polygon_set_element(poly, 1, 3, 1, 1.0f);
	nemocook_attach_polygon(cook, poly);

	context->motiontex = tex = nemocook_texture_create();
	nemocook_texture_assign(tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, width, height);
	context->motionpoly = poly = nemocook_polygon_create();
	nemocook_polygon_set_texture(poly, tex);
	nemocook_polygon_set_shader(poly, shader);
	nemocook_polygon_set_type(poly, GL_TRIANGLE_STRIP);
	nemocook_polygon_set_count(poly, 4);
	nemocook_polygon_set_buffer(poly, 0, 2);
	nemocook_polygon_set_buffer(poly, 1, 2);
	nemocook_polygon_set_element(poly, 0, 0, 0, -0.45f);
	nemocook_polygon_set_element(poly, 0, 0, 1, -0.45f);
	nemocook_polygon_set_element(poly, 0, 1, 0, 0.45f);
	nemocook_polygon_set_element(poly, 0, 1, 1, -0.45f);
	nemocook_polygon_set_element(poly, 0, 2, 0, -0.45f);
	nemocook_polygon_set_element(poly, 0, 2, 1, -0.95f);
	nemocook_polygon_set_element(poly, 0, 3, 0, 0.45f);
	nemocook_polygon_set_element(poly, 0, 3, 1, -0.95f);
	nemocook_polygon_set_element(poly, 1, 0, 0, 0.0f);
	nemocook_polygon_set_element(poly, 1, 0, 1, 0.0f);
	nemocook_polygon_set_element(poly, 1, 1, 0, 1.0f);
	nemocook_polygon_set_element(poly, 1, 1, 1, 0.0f);
	nemocook_polygon_set_element(poly, 1, 2, 0, 0.0f);
	nemocook_polygon_set_element(poly, 1, 2, 1, 1.0f);
	nemocook_polygon_set_element(poly, 1, 3, 0, 1.0f);
	nemocook_polygon_set_element(poly, 1, 3, 1, 1.0f);
	nemocook_attach_polygon(cook, poly);

	image = pixman_load_image(imagepath, width, height);
	nemocook_texture_upload(context->backtex,
			pixman_image_get_data(image));
	pixman_image_unref(image);

	context->play = play = nemoplay_create();
	nemoplay_load_media(play, videopath);

	context->decoderback = nemoplay_decoder_create(play);
	context->audioback = nemoplay_audio_create_by_ao(play);
	context->videoback = nemoplay_video_create_by_timer(play, tool);
	nemoplay_video_set_texture(context->videoback,
			nemocook_texture_get(context->videotex),
			width, height);
	nemoplay_video_set_update(context->videoback, nemocook_dispatch_video_update);
	nemoplay_video_set_done(context->videoback, nemocook_dispatch_video_done);
	nemoplay_video_set_data(context->videoback, context);

	context->motion = play = nemoplay_create();
	nemoplay_load_media(play, motionpath);

	context->box = nemoplay_box_create(nemoplay_get_video_framecount(play));
	context->extractor = nemoplay_extractor_create(play, context->box);

	context->player = nemoplay_shader_create();
	nemoplay_shader_set_format(context->player,
			nemoplay_get_pixel_format(play));
	nemoplay_shader_set_texture(context->player,
			nemoplay_get_video_width(play),
			nemoplay_get_video_height(play));
	nemoplay_shader_set_viewport(context->player,
			nemocook_texture_get(context->motiontex),
			width, height);

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
