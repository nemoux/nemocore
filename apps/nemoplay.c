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

#define	NEMOPLAY_SEEK_ENABLE						(0)

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

	int32_t width, height;

	pthread_mutex_t lock;

	gint video_width;
	gint video_height;
	guint8 *video_buffer;

	int is_background;
	int is_fullscreen;

	int is_audio_only;

	float gx, gy;

	int64_t position;

	uint32_t pid;
	int32_t volume;

	double alpha;
	int is_alpha_mode;
};

static void nemoplay_dispatch_subtitle(GstElement *base, guint8 *data, gsize size, gpointer userdata)
{
	nemolog_message("PLAY", "{%s}\n", data);
}

static void nemoplay_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct playcontext *context = (struct playcontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (context->is_background != 0)
		return;

	if (id == 1) {
		nemotale_event_update_node_taps(tale, node, event, type);

		if (nemotale_is_touch_down(tale, event, type) ||
				nemotale_is_touch_up(tale, event, type)) {
			if (nemotale_is_single_tap(tale, event, type)) {
				nemocanvas_move(context->canvas, event->taps[0]->serial);
			} else if (nemotale_is_double_taps(tale, event, type)) {
				nemocanvas_pick(context->canvas,
						event->taps[0]->serial,
						event->taps[1]->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
			} else if (nemotale_is_many_taps(tale, event, type)) {
				nemotale_event_update_faraway_taps(tale, event);

				nemocanvas_pick(context->canvas,
						event->tap0->serial,
						event->tap1->serial,
						(1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE));
#if	NEMOPLAY_SEEK_ENABLE
			} else if (nemotale_is_triple_taps(tale, event, type)) {
				context->position = nemogst_get_position(context->gst);

				context->gx = event->x;
				context->gy = event->y;
#endif
			}
#if NEMOPLAY_SEEK_ENABLE
		} else if (nemotale_is_motion_event(tale, event, type)) {
			if (nemotale_is_triple_taps(tale, event, type)) {
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
					if (context->is_alpha_mode == 0) {
						context->volume = MIN(context->volume + 1, 100);

						nemosound_set_volume(context->tool, context->pid, context->volume);

						context->gx = event->x;
						context->gy = event->y;
					} else {
						context->alpha = MIN(context->alpha + 0.001f, 1.0f);

						nemotale_node_set_alpha(context->node, context->alpha);
						nemotale_node_damage_all(context->node);
					}
				} else if (context->gy + NEMOPLAY_SLIDE_DISTANCE_MIN < event->taps[2]->y) {
					if (context->is_alpha_mode == 0) {
						context->volume = MAX(context->volume - 1, 0);

						nemosound_set_volume(context->tool, context->pid, context->volume);

						context->gx = event->x;
						context->gy = event->y;
					} else {
						context->alpha = MAX(context->alpha - 0.001f, 0.0f);

						nemotale_node_set_alpha(context->node, context->alpha);
						nemotale_node_damage_all(context->node);
					}
				}
			}
#endif
		}

#if	0
		if (nemotale_is_long_press(tale, event, type)) {
			if (nemogst_is_playing_media(context->gst) != 0) {
				nemogst_pause_media(context->gst);
			}
		}

		if (nemotale_is_single_click(tale, event, type)) {
			if (nemogst_is_playing_media(context->gst) == 0) {
				nemogst_play_media(context->gst);
			}

			context->is_alpha_mode = !context->is_alpha_mode;
		}
#endif
	}
}

static void nemoplay_dispatch_tale_timer(struct nemotimer *timer, void *data)
{
	struct playcontext *context = (struct playcontext *)data;

	nemotimer_set_timeout(timer, 500);

	nemotale_push_timer_event(context->tale, time_current_msecs());
}

static void nemoplay_dispatch_video_frame(GstElement *base, guint8 *data, gint width, gint height, GstVideoFormat format, gpointer userdata)
{
	struct playcontext *context = (struct playcontext *)userdata;

	pthread_mutex_lock(&context->lock);

	context->video_buffer = data;
	context->video_width = width;
	context->video_height = height;

	pthread_mutex_unlock(&context->lock);
}

static void nemoplay_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct playcontext *context = (struct playcontext *)nemotale_get_userdata(tale);
	int32_t width, height;
	void *data;

	if (nemogst_is_done_media(context->gst) != 0)
		nemogst_replay_media(context->gst);

	pthread_mutex_lock(&context->lock);
	width = context->video_width;
	height = context->video_height;
	data = context->video_buffer;
	context->video_buffer = NULL;
	pthread_mutex_unlock(&context->lock);

	if (data != NULL) {
		if (context->width != width || context->height != height) {
			nemotool_resize_egl_canvas(context->ecanvas, width, height);
			nemotale_resize(context->tale, width, height);
			nemotale_node_resize_pixman(context->node, width, height);
			nemotale_node_opaque(context->node, 0, 0, width, height);

			context->width = width;
			context->height = height;
		}

		nemotale_node_attach_pixman(context->node, data, width, height);

		nemogst_sink_set_property(context->gst, "frame-done", 1);
	}

	nemocanvas_feedback(context->canvas);

	nemotale_composite_egl(context->tale, NULL);
}

static void nemoplay_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct playcontext *context = (struct playcontext *)nemotale_get_userdata(tale);

	if (width == 0 || height == 0)
		return;

	if (width < nemotale_get_minimum_width(tale) || height < nemotale_get_minimum_height(tale)) {
		nemotool_exit(context->tool);
		return;
	}

	if (context->is_audio_only == 0) {
		if (fixed == 0) {
			height = width / nemogst_get_video_aspect_ratio(context->gst);
		}

		nemogst_resize_video(context->gst, width, height);

		if (nemogst_is_playing_media(context->gst) == 0) {
			nemogst_set_next_step(context->gst, 1, 1.0f);
		}
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",					required_argument,	NULL,		'f' },
		{ "subtitle",			required_argument,	NULL,		's' },
		{ "width",				required_argument,	NULL,		'w' },
		{ "height",				required_argument,	NULL,		'h' },
		{ "background",		no_argument,				NULL,		'b' },
		{ "fullscreen",		no_argument,				NULL,		'u' },
		{ "log",					required_argument,	NULL,		'l' },
		{ 0 }
	};

	GMainLoop *gmainloop;
	struct playcontext *context;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct eglcanvas *ecanvas;
	struct nemotimer *timer;
	struct nemotale *tale;
	struct talenode *node;
	char *filepath = NULL;
	char *subtitlepath = NULL;
	char *uri;
	int32_t width = 0, height = 0;
	int is_background = 0;
	int is_fullscreen = 0;
	int opt;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "f:s:w:h:bul:", options, NULL)) {
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

			case 'u':
				is_fullscreen = 1;
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
	context->is_fullscreen = is_fullscreen;

	context->pid = getpid();
	context->volume = 50;

	context->alpha = 1.0f;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto out1;
	nemotool_connect_wayland(tool, NULL);

	context->gst = nemogst_create();
	if (context->gst == NULL)
		goto out2;

	if (pthread_mutex_init(&context->lock, NULL) != 0)
		goto out3;

	asprintf(&uri, "file://%s", filepath);

	nemogst_load_media_info(context->gst, uri);

	context->is_audio_only = nemogst_get_video_width(context->gst) == 0 || nemogst_get_video_height(context->gst) == 0;

	if (width == 0 || height == 0) {
		if (context->is_audio_only == 0) {
			width = 480;
			height = 480 / nemogst_get_video_aspect_ratio(context->gst);
		} else {
			width = 480;
			height = 480;
		}
	}

	context->width = width;
	context->height = height;

	context->egl = nemotool_create_egl(tool);

	context->ecanvas = ecanvas = nemotool_create_egl_canvas(context->egl, width, height);
	context->canvas = canvas = NTEGL_CANVAS(ecanvas);
	nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_fullscreen_type(canvas, (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK) | (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH));
	if (context->is_background != 0)
		nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	if (context->is_fullscreen != 0)
		nemocanvas_set_fullscreen(canvas, 0);
	else
		nemocanvas_set_anchor(canvas, -0.5f, -0.5f);
	nemocanvas_set_dispatch_frame(canvas, nemoplay_dispatch_canvas_frame);
	nemocanvas_set_dispatch_resize(canvas, nemoplay_dispatch_canvas_resize);
	nemocanvas_set_max_size(canvas, UINT32_MAX, UINT32_MAX);
	nemocanvas_set_input_type(canvas, NEMO_SURFACE_INPUT_TYPE_TOUCH);

	context->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(context->egl),
				NTEGL_CONTEXT(context->egl),
				NTEGL_CONFIG(context->egl),
				(EGLNativeWindowType)NTEGL_WINDOW(ecanvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, canvas, nemoplay_dispatch_tale_event);
	nemotale_set_userdata(tale, context);

	context->node = node = nemotale_node_create_pixman(width, height);
	nemotale_attach_node(tale, node);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);

	if (context->is_audio_only == 0) {
		nemogst_prepare_mini_sink(context->gst,
				nemoplay_dispatch_video_frame,
				context);
		nemogst_set_media_path(context->gst, uri);

		if (subtitlepath != NULL) {
			nemogst_prepare_nemo_subsink(context->gst, nemoplay_dispatch_subtitle, context);
			nemogst_set_subtitle_path(context->gst, subtitlepath);
		}

		nemogst_resize_video(context->gst, width, height);
	} else {
		nemogst_prepare_audio_sink(context->gst);
		nemogst_set_media_path(context->gst, uri);

		nemotale_node_fill_pixman(context->node,
				random_get_double(0.0f, 1.0f),
				random_get_double(0.0f, 1.0f),
				random_get_double(0.0f, 1.0f),
				random_get_double(0.0f, 1.0f));
	}

	timer = nemotimer_create(context->tool);
	nemotimer_set_callback(timer, nemoplay_dispatch_tale_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, context);

	nemogst_play_media(context->gst);

	nemocanvas_dispatch_frame(context->canvas);

	gmainloop = g_main_loop_new(NULL, FALSE);
	nemoglib_run_tool(gmainloop, context->tool);
	g_main_loop_unref(gmainloop);

	nemotool_destroy_egl_canvas(context->ecanvas);
	nemotool_destroy_egl(context->egl);

	nemolog_message("PLAY", "done '%s' media file...\n", filepath);

	free(uri);

	pthread_mutex_destroy(&context->lock);

	nemotimer_destroy(timer);

out3:
	nemogst_destroy(context->gst);

out2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

out1:
	free(context);

	return 0;
}
