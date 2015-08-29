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
#include <nemomisc.h>

#define	NEMOPLAY_SEEK_ENABLE						(0)

#define	NEMOPLAY_SLIDE_DISTANCE_MIN			(5.0f)
#define	NEMOPLAY_SLIDE_FRAME_TIME				(10000000000)

struct playcontext {
	struct nemotool *tool;
	struct nemogst *gst;

	struct nemocanvas *canvas;
	struct eglcanvas *ecanvas;

	struct nemotale *tale;
	struct talenode *node;

	int is_fullscreen;
	int is_background;

	float gx, gy;

	int64_t position;
	double volume;

	char *snddev;
};

static void nemoplay_dispatch_subtitle(GstElement *base, guint8 *buffer, gsize size, gpointer data)
{
	NEMO_DEBUG("[%s]\n", buffer);
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
				if (context->is_fullscreen == 0) {
					nemocanvas_move(context->canvas, event->taps[0]->serial);
				}
			} else if (event->tapcount == 2) {
				if (context->is_fullscreen != 0) {
					nemocanvas_unset_fullscreen(context->canvas);

					context->is_fullscreen = 0;
				}

				nemocanvas_pick(context->canvas,
						event->taps[0]->serial,
						event->taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE));
			} else if (event->tapcount == 3) {
#if	NEMOPLAY_SEEK_ENABLE
				nemocanvas_miss(context->canvas);

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
					nemogst_pause_video(context->gst);
				} else {
					nemogst_play_video(context->gst);
				}
			} else if (event->tapcount == 2) {
				nemogst_dump_state(context->gst);
			} else if (event->tapcount == 4) {
				if (context->is_fullscreen == 0) {
					nemocanvas_set_fullscreen(context->canvas);

					context->is_fullscreen = 1;
				}
			}
		}
	}
}

static void nemoplay_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct playcontext *context = (struct playcontext *)nemocanvas_get_userdata(canvas);

	if (width == 0 || height == 0)
		return;

	if (width < 200 || height < 200)
		nemotool_exit(context->tool);

	nemogst_resize_video(context->gst, width, height);

	nemotool_resize_egl_canvas(context->ecanvas, width, height);
	nemotale_resize(context->tale, width, height);
	nemotale_node_resize_pixman(context->node, width, height);
	nemotale_node_opaque(context->node, 0, 0, width, height);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemoplay_dispatch_canvas_screen(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height, int left)
{
	struct playcontext *context = (struct playcontext *)nemocanvas_get_userdata(canvas);
}

static void nemoplay_dispatch_canvas_sound(struct nemocanvas *canvas, const char *device, int left)
{
#ifdef NEMOUX_WITH_ALSA_MULTINODE
	struct playcontext *context = (struct playcontext *)nemocanvas_get_userdata(canvas);

	if (context->snddev == NULL) {
		if (left == 0)
			nemogst_set_audio_device(context->gst, device);
		else
			nemogst_put_audio_device(context->gst, device);
	}
#endif
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",					required_argument,	NULL,		'f' },
		{ "subtitle",			required_argument,	NULL,		's' },
		{ "sounddevice",	required_argument,	NULL,		'd' },
		{ "width",				required_argument,	NULL,		'w' },
		{ "height",				required_argument,	NULL,		'h' },
		{ "background",		no_argument,				NULL,		'b' },
		{ 0 }
	};

	GMainLoop *gmainloop;
	struct playcontext *context;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct eglcontext *egl;
	struct eglcanvas *ecanvas;
	struct nemotale *tale;
	struct talenode *node;
	struct pathone *group, *one;
	char *filepath = NULL;
	char *subtitlepath = NULL;
	char *snddev = NULL;
	char *uri;
	int32_t width = 0, height = 0;
	int is_background = 0;
	int opt;

	while (opt = getopt_long(argc, argv, "f:s:d:w:h:b", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 's':
				subtitlepath = strdup(optarg);
				break;

			case 'd':
				snddev = strdup(optarg);
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

	context->canvas = canvas = nemocanvas_create(tool);
	nemocanvas_set_userdata(canvas, context);
	nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	if (context->is_background != 0)
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_anchor(canvas, -0.5f, -0.5f);
	nemocanvas_set_dispatch_resize(canvas, nemoplay_dispatch_canvas_resize);
	nemocanvas_set_dispatch_screen(canvas, nemoplay_dispatch_canvas_screen);
	nemocanvas_set_dispatch_sound(canvas, nemoplay_dispatch_canvas_sound);

	context->gst = nemogst_create();
	if (context->gst == NULL)
		goto out2;

	asprintf(&uri, "file://%s", filepath);

	nemogst_prepare_nemo_sink(context->gst,
			nemotool_get_display(tool),
			nemotool_get_shm(tool),
			nemotool_get_formats(tool),
			nemocanvas_get_surface(canvas));
	nemogst_set_video_path(context->gst, uri);

	free(uri);

	if (subtitlepath != NULL) {
		nemogst_prepare_nemo_subsink(context->gst, nemoplay_dispatch_subtitle, context);
		nemogst_set_subtitle_path(context->gst, subtitlepath);
	}

	if (width == 0 || height == 0) {
		width = 320 * nemogst_get_video_aspect_ratio(context->gst);
		height = 320;
	}

	nemogst_resize_video(context->gst, width, height);

	egl = nemotool_create_egl(tool);

	context->ecanvas = ecanvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(ecanvas), NEMO_SHELL_SURFACE_TYPE_OVERLAY);
	nemocanvas_set_parent(NTEGL_CANVAS(ecanvas), canvas);

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(ecanvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(ecanvas), nemoplay_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	context->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_attach_node(tale, node);
	nemotale_node_opaque(node, 0, 0, width, height);

	nemotale_composite_egl(context->tale, NULL);

#ifdef NEMOUX_WITH_ALSA_MULTINODE
	if (snddev != NULL) {
		nemogst_set_audio_device(context->gst, snddev);

		context->snddev = snddev;
	}
#endif

	nemogst_play_video(context->gst);

	gmainloop = g_main_loop_new(NULL, FALSE);

	nemoglib_run_tool(gmainloop, context->tool);

	g_main_loop_unref(gmainloop);

	nemogst_destroy(context->gst);

out2:
	nemocanvas_destroy(canvas);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(context);

	return 0;
}
