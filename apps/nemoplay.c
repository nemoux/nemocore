#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <glib.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <gsthelper.h>
#include <talehelper.h>
#include <glibhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define	NEMOPLAY_SEEK_ENABLE						(1)

#define	NEMOPLAY_SLIDE_DISTANCE_MIN			(5.0f)
#define	NEMOPLAY_SLIDE_FRAME_TIME				(10000000000)

struct playcontext {
	struct nemotool *tool;
	struct nemogst *gst;

	struct eglcontext *egl;

	struct nemocanvas *canvas;
	struct eglcanvas *ecanvas;

	struct nemotale *tale;
	struct talenode *node;

	int is_background;

	int is_audio_only;

	float gx, gy;

	int64_t position;
	double volume;
};

static void nemoplay_dispatch_subtitle(GstElement *base, guint8 *buffer, gsize size, gpointer data)
{
	nemolog_message("PLAY", "{%s}\n", buffer);
}

static void nemoplay_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct playcontext *context = (struct playcontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (context->is_background != 0)
		return;

	if (id == 1) {
		if (nemotale_is_touch_down(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
				nemocanvas_move(context->canvas, event->taps[0]->serial);
			} else if (event->tapcount == 2) {
				nemocanvas_pick(context->canvas,
						event->taps[0]->serial,
						event->taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
			} else if (event->tapcount == 3) {
#if	NEMOPLAY_SEEK_ENABLE
				context->position = nemogst_get_position(context->gst);

				context->gx = event->x;
				context->gy = event->y;
#endif
			}
		} else if (nemotale_is_motion_event(tale, event, type)) {
#if NEMOPLAY_SEEK_ENABLE
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 3) {
				if (context->gx - NEMOPLAY_SLIDE_DISTANCE_MIN > event->taps[2]->x) {
					if (context->position != 0) {
						context->position = MAX(context->position - NEMOPLAY_SLIDE_FRAME_TIME, 0);

						nemogst_set_position(context->gst, context->position);

						context->gx = event->x;
						context->gy = event->y;
					}
				} else if (context->gx + NEMOPLAY_SLIDE_DISTANCE_MIN < event->taps[2]->x) {
					if (context->position != 0) {
						context->position = context->position + NEMOPLAY_SLIDE_FRAME_TIME;

						nemogst_set_position(context->gst, context->position);

						context->gx = event->x;
						context->gy = event->y;
					}
				} else if (context->gy - NEMOPLAY_SLIDE_DISTANCE_MIN > event->taps[2]->y) {
					if (context->volume < 1.0f) {
						context->volume = MIN(context->volume + 0.1f, 1.0f);

						nemogst_set_audio_volume(context->gst, context->volume);

						context->gx = event->x;
						context->gy = event->y;
					}
				} else if (context->gy + NEMOPLAY_SLIDE_DISTANCE_MIN < event->taps[2]->y) {
					if (context->volume > 0.0f) {
						context->volume = MAX(context->volume - 0.1f, 0.0f);

						nemogst_set_audio_volume(context->gst, context->volume);

						context->gx = event->x;
						context->gy = event->y;
					}
				}
			}
#endif
		} else if (nemotale_is_single_click(tale, event, type) != 0) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (event->tapcount == 1) {
				if (nemogst_is_playing(context->gst)) {
					nemogst_pause_media(context->gst);
				} else {
					nemogst_play_media(context->gst);
				}
			} else if (event->tapcount == 2) {
				nemogst_dump_state(context->gst);
			}
		}
	}
}

static void nemoplay_dispatch_video_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct playcontext *context = (struct playcontext *)nemocanvas_get_userdata(canvas);
	struct nemotale *tale = context->tale;

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_minimum_width(tale) || height < nemotale_get_minimum_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

	if (fixed == 0) {
		height = width / nemogst_get_video_aspect_ratio(context->gst);
	}

	nemogst_resize_video(context->gst, width, height);

	nemotool_resize_egl_canvas(context->ecanvas, width, height);
	nemotale_resize(context->tale, width, height);
	nemotale_node_resize_pixman(context->node, width, height);
	nemotale_node_opaque(context->node, 0, 0, width, height);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemoplay_dispatch_audio_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct playcontext *context = (struct playcontext *)nemotale_get_userdata(tale);

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else {
		nemocanvas_feedback(canvas);
	}

	nemotale_node_fill_pixman(context->node,
			random_get_double(0.0f, 1.0f),
			random_get_double(0.0f, 1.0f),
			random_get_double(0.0f, 1.0f),
			random_get_double(0.0f, 1.0f));

	nemotale_node_damage_all(context->node);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemoplay_dispatch_audio_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct playcontext *context = (struct playcontext *)nemotale_get_userdata(tale);

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_minimum_width(tale) || height < nemotale_get_minimum_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

	nemotool_resize_egl_canvas(context->ecanvas, width, height);
	nemotale_resize(context->tale, width, height);
	nemotale_node_resize_pixman(context->node, width, height);
	nemotale_node_opaque(context->node, 0, 0, width, height);

	nemocanvas_dispatch_frame(canvas);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",					required_argument,	NULL,		'f' },
		{ "subtitle",			required_argument,	NULL,		's' },
		{ "width",				required_argument,	NULL,		'w' },
		{ "height",				required_argument,	NULL,		'h' },
		{ "background",		no_argument,				NULL,		'b' },
		{ "log",					required_argument,	NULL,		'l' },
		{ 0 }
	};

	GMainLoop *gmainloop;
	struct playcontext *context;
	struct nemotool *tool;
	struct nemocanvas *canvas = NULL;
	struct eglcanvas *ecanvas;
	struct nemotale *tale;
	struct talenode *node;
	struct pathone *group, *one;
	char *filepath = NULL;
	char *subtitlepath = NULL;
	char *uri;
	int32_t width = 0, height = 0;
	int is_background = 0;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "f:s:w:h:bl:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 's':
				subtitlepath = strdup(optarg);
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

			case 'l':
				nemolog_open_socket(optarg);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	nemolog_message("PLAY", "play '%s' media file...\n", filepath);

	gst_init(&argc, &argv);

	context = (struct playcontext *)malloc(sizeof(struct playcontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct playcontext));

	context->is_background = is_background;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out1;
	nemotool_connect_wayland(tool, NULL);

	context->egl = nemotool_create_egl(tool);

	context->gst = nemogst_create();
	if (context->gst == NULL)
		goto out2;

	asprintf(&uri, "file://%s", filepath);

	nemogst_load_media_info(context->gst, uri);

	context->is_audio_only = nemogst_get_video_width(context->gst) == 0 || nemogst_get_video_height(context->gst) == 0;
	if (context->is_audio_only == 0) {
		context->canvas = canvas = nemocanvas_create(tool);
		nemocanvas_set_userdata(canvas, context);
		nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
		nemocanvas_set_fullscreen_type(canvas, NEMO_SURFACE_FULLSCREEN_TYPE_PICK);
		if (context->is_background != 0)
			nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
		nemocanvas_set_anchor(canvas, -0.5f, -0.5f);
		nemocanvas_set_dispatch_resize(canvas, nemoplay_dispatch_video_resize);

		nemogst_prepare_nemo_sink(context->gst,
				nemotool_get_display(tool),
				nemotool_get_shm(tool),
				nemotool_get_formats(tool),
				nemocanvas_get_surface(canvas));
		nemogst_set_media_path(context->gst, uri);

		if (subtitlepath != NULL) {
			nemogst_prepare_nemo_subsink(context->gst, nemoplay_dispatch_subtitle, context);
			nemogst_set_subtitle_path(context->gst, subtitlepath);
		}

		if (width == 0 || height == 0) {
			width = 480;
			height = 480 / nemogst_get_video_aspect_ratio(context->gst);
		}

		nemogst_resize_video(context->gst, width, height);

		context->ecanvas = ecanvas = nemotool_create_egl_canvas(context->egl, width, height);
		nemocanvas_set_nemosurface(NTEGL_CANVAS(ecanvas), NEMO_SHELL_SURFACE_TYPE_OVERLAY);
		nemocanvas_set_parent(NTEGL_CANVAS(ecanvas), canvas);

		context->tale = tale = nemotale_create_gl();
		nemotale_set_backend(tale,
				nemotale_create_egl(
					NTEGL_DISPLAY(context->egl),
					NTEGL_CONTEXT(context->egl),
					NTEGL_CONFIG(context->egl),
					(EGLNativeWindowType)NTEGL_WINDOW(ecanvas)));
		nemotale_resize(tale, width, height);

		nemotale_attach_canvas(tale, NTEGL_CANVAS(ecanvas), nemoplay_dispatch_tale_event);
		nemotale_set_userdata(tale, context);

		context->node = node = nemotale_node_create_pixman(width, height);
		nemotale_node_set_id(node, 1);
		nemotale_attach_node(tale, node);
		nemotale_node_opaque(node, 0, 0, width, height);

		nemotale_composite_egl(context->tale, NULL);
	} else {
		nemogst_prepare_audio_sink(context->gst);
		nemogst_set_media_path(context->gst, uri);

		if (width == 0 || height == 0) {
			width = 480;
			height = 480;
		}

		context->ecanvas = ecanvas = nemotool_create_egl_canvas(context->egl, width, height);
		context->canvas = NTEGL_CANVAS(ecanvas);
		nemocanvas_set_nemosurface(NTEGL_CANVAS(ecanvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
		nemocanvas_set_fullscreen_type(NTEGL_CANVAS(ecanvas), NEMO_SURFACE_FULLSCREEN_TYPE_PICK);
		if (context->is_background != 0)
			nemocanvas_set_layer(NTEGL_CANVAS(ecanvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
		nemocanvas_set_dispatch_frame(NTEGL_CANVAS(ecanvas), nemoplay_dispatch_audio_frame);
		nemocanvas_set_dispatch_resize(NTEGL_CANVAS(ecanvas), nemoplay_dispatch_audio_resize);

		context->tale = tale = nemotale_create_gl();
		nemotale_set_backend(tale,
				nemotale_create_egl(
					NTEGL_DISPLAY(context->egl),
					NTEGL_CONTEXT(context->egl),
					NTEGL_CONFIG(context->egl),
					(EGLNativeWindowType)NTEGL_WINDOW(ecanvas)));
		nemotale_resize(tale, width, height);

		nemotale_attach_canvas(tale, NTEGL_CANVAS(ecanvas), nemoplay_dispatch_tale_event);
		nemotale_set_userdata(tale, context);

		context->node = node = nemotale_node_create_pixman(width, height);
		nemotale_node_set_id(node, 1);
		nemotale_attach_node(tale, node);
		nemotale_node_opaque(node, 0, 0, width, height);

		nemocanvas_dispatch_frame(NTEGL_CANVAS(ecanvas));
	}

	free(uri);

	nemogst_set_repeat(context->gst, UINT32_MAX);
	nemogst_play_media(context->gst);

	gmainloop = g_main_loop_new(NULL, FALSE);

	nemoglib_run_tool(gmainloop, context->tool);

	g_main_loop_unref(gmainloop);

	nemogst_destroy(context->gst);

	if (context->is_audio_only == 0) {
		nemotool_destroy_egl_canvas(context->ecanvas);
		nemocanvas_destroy(context->canvas);
	} else {
		nemotool_destroy_egl_canvas(context->ecanvas);
	}

	nemotool_destroy_egl(context->egl);

	nemolog_message("PLAY", "done '%s' media file...\n", filepath);

out2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(context);

	return 0;
}
