#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <pthread.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemosound.h>
#include <nemotimer.h>
#include <nemoglib.h>
#include <gsthelper.h>
#include <talehelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct videocontext {
	struct nemotool *tool;
	struct nemogst *gst;

	struct eglcontext *egl;

	struct nemocanvas *canvas;
	struct eglcanvas *ecanvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	int is_background;
};

static void nemovideo_dispatch_tale_event(struct nemotale *tale, struct talenode *node, struct taleevent *event)
{
	struct videocontext *context = (struct videocontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (context->is_background != 0)
		return;

	if (id == 1) {
		nemotale_event_update_taps_by_node(tale, node, event);

		if (nemotale_event_is_touch_down(tale, event) || nemotale_event_is_touch_up(tale, event)) {
			if (nemotale_event_is_single_tap(tale, event)) {
				nemocanvas_move(context->canvas, nemotale_event_get_serial_on(event, 0));
			} else if (nemotale_event_is_many_taps(tale, event)) {
				uint32_t serial0, serial1;

				nemotale_event_get_distant_tapserials(tale, event, &serial0, &serial1);

				nemocanvas_pick(context->canvas,
						serial0, serial1,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
			}
		}
	}
}

static void nemovideo_dispatch_tale_timer(struct nemotimer *timer, void *data)
{
	struct videocontext *context = (struct videocontext *)data;

	nemotimer_set_timeout(timer, 500);

	if (nemogst_is_done_media(context->gst) != 0)
		nemogst_replay_media(context->gst);

	nemotale_push_timer_event(context->tale, time_current_msecs());
}

static void nemovideo_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct videocontext *context = (struct videocontext *)nemotale_get_userdata(tale);

	nemotale_composite_egl(tale, NULL);
}

static void nemovideo_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct videocontext *context = (struct videocontext *)nemotale_get_userdata(tale);

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_close_width(tale) || height < nemotale_get_close_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

	if (fixed == 0)
		height = width / nemogst_get_video_aspect_ratio(context->gst);

	nemogst_resize_media(context->gst, width, height);

	if (nemogst_is_playing_media(context->gst) == 0)
		nemogst_set_next_step(context->gst, 1, 1.0f);

	nemotool_resize_egl_canvas(context->ecanvas, width, height);
	nemotale_resize(context->tale, width, height);
	nemotale_node_resize_pixman(context->node, width, height);
	nemotale_node_opaque(context->node, 0, 0, width, height);

	context->width = width;
	context->height = height;
}

static void nemovideo_dispatch_canvas_transform(struct nemocanvas *canvas, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",					required_argument,	NULL,		'f' },
		{ "width",				required_argument,	NULL,		'w' },
		{ "height",				required_argument,	NULL,		'h' },
		{ "background",		no_argument,				NULL,		'b' },
		{ 0 }
	};

	GMainLoop *gmainloop;
	struct videocontext *context;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct eglcanvas *ecanvas;
	struct nemotimer *timer;
	struct nemotale *tale;
	struct talenode *node;
	char *filepath = NULL;
	char *uri;
	int32_t width = 0, height = 0;
	int is_background = 0;
	int has_video;
	int opt;

	while (opt = getopt_long(argc, argv, "f:w:h:b", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'b':
				is_background = 1;
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	nemolog_message("PLAY", "play '%s' media file...\n", filepath);

	gst_init(&argc, &argv);

	context = (struct videocontext *)malloc(sizeof(struct videocontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct videocontext));

	context->is_background = is_background;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out1;
	nemotool_connect_wayland(tool, NULL);

	context->gst = nemogst_create();
	if (context->gst == NULL)
		goto out2;

	asprintf(&uri, "file://%s", filepath);
	has_video = nemogst_prepare_media(context->gst, uri);
	free(uri);

	if (has_video == 0)
		goto out3;

	context->width = width = width != 0 ? width : 480;
	context->height = height = height != 0 ? height : 480 / nemogst_get_video_aspect_ratio(context->gst);

	context->egl = nemotool_create_egl(tool);

	context->ecanvas = ecanvas = nemotool_create_egl_canvas(context->egl, width, height);
	context->canvas = canvas = NTEGL_CANVAS(ecanvas);
	nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_fullscreen_type(canvas, (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK) | (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH));
	if (context->is_background != 0) {
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
		nemocanvas_set_fullscreen(canvas, 0);
	} else {
		nemocanvas_set_anchor(canvas, -0.5f, -0.5f);
	}
	nemocanvas_set_dispatch_frame(canvas, nemovideo_dispatch_canvas_frame);
	nemocanvas_set_dispatch_resize(canvas, nemovideo_dispatch_canvas_resize);
	nemocanvas_set_dispatch_transform(canvas, nemovideo_dispatch_canvas_transform);
	nemocanvas_set_max_size(canvas, UINT32_MAX, UINT32_MAX);
	nemocanvas_set_state(canvas, "layer");
	nemocanvas_set_state(canvas, "opaque");
	nemocanvas_put_state(canvas, "keypad");

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(context->egl),
				NTEGL_CONTEXT(context->egl),
				NTEGL_CONFIG(context->egl),
				(EGLNativeWindowType)NTEGL_WINDOW(ecanvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, canvas, nemovideo_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	context->node = node = nemotale_node_create_pixman(width, height);
	nemotale_attach_node(tale, node);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);

	nemocanvas_dispatch_frame(context->canvas);

	timer = nemotimer_create(context->tool);
	nemotimer_set_callback(timer, nemovideo_dispatch_tale_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, context);

	nemogst_prepare_sink(context->gst,
			nemotool_get_display(tool),
			nemotool_get_shm(tool),
			nemotool_get_formats(tool),
			nemocanvas_get_surface(canvas));
	nemogst_resize_media(context->gst, width, height);

	nemogst_play_media(context->gst);

	gmainloop = g_main_loop_new(NULL, FALSE);
	nemoglib_run_tool(gmainloop, context->tool);
	g_main_loop_unref(gmainloop);

	nemogst_pause_media(context->gst);

	nemotimer_destroy(timer);

	nemotool_destroy_egl_canvas(context->ecanvas);
	nemotool_destroy_egl(context->egl);

	nemolog_message("PLAY", "done '%s' media file...\n", filepath);

out3:
	nemogst_destroy(context->gst);

out2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(context);

	return 0;
}
