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

#include <nemoplay.h>
#include <playback.h>
#include <nemocook.h>
#include <nemobus.h>
#include <nemoaction.h>
#include <nemofs.h>
#include <nemomisc.h>

struct nemoart {
	struct nemotool *tool;
	struct nemocanvas *canvas;

	struct nemoaction *action;

	int width, height;
	int vertex;

	char *threads;

	struct fsdir *contents;
	int icontents;

	struct cookegl *egl;

	struct nemoplay *play;
	struct playdecoder *decoderback;
	struct playaudio *audioback;
	struct playvideo *videoback;
	struct playshader *shader;

	struct nemobus *bus;
};

static int nemoart_play_video(struct nemoart *art, const char *url);

static void nemoart_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);

	if (width == 0 || height == 0)
		return;

	art->width = width;
	art->height = height;

	nemocanvas_egl_resize(art->canvas, width, height);
	nemocanvas_opaque(art->canvas, 0, 0, width, height);

	nemocook_egl_resize(art->egl, width, height);

	if (art->play != NULL) {
		nemoplay_video_set_texture(art->videoback, 0, width, height);

		nemocook_egl_make_current(art->egl);
		nemoplay_shader_dispatch(art->shader);
		nemocook_egl_swap_buffers(art->egl);
	}
}

static void nemoart_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);

	if (art->play != NULL) {
		nemocook_egl_make_current(art->egl);
		nemoplay_shader_dispatch(art->shader);
		nemocook_egl_swap_buffers(art->egl);
	}
}

static int nemoart_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);
	struct actiontap *tap;

	if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		tap = nemoaction_tap_create(art->action);
		nemoaction_tap_set_tx(tap, event->x);
		nemoaction_tap_set_ty(tap, event->y);
		nemoaction_tap_set_device(tap, event->device);
		nemoaction_tap_set_serial(tap, event->serial);
		nemoaction_tap_clear(tap, event->gx, event->gy);
		nemoaction_tap_dispatch_event(art->action, tap, NEMOACTION_TAP_DOWN_EVENT);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		tap = nemoaction_get_tap_by_device(art->action, event->device);
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, event->x);
			nemoaction_tap_set_ty(tap, event->y);
			nemoaction_tap_dispatch_event(art->action, tap, NEMOACTION_TAP_UP_EVENT);
			nemoaction_tap_destroy(tap);
		}
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		tap = nemoaction_get_tap_by_device(art->action, event->device);
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, event->x);
			nemoaction_tap_set_ty(tap, event->y);
			nemoaction_tap_trace(tap, event->gx, event->gy);
			nemoaction_tap_dispatch_event(art->action, tap, NEMOACTION_TAP_MOTION_EVENT);
		}
	}

	return 0;
}

static int nemoart_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	nemotool_exit(canvas->tool);

	return 1;
}

static void nemoart_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct nemoart *art = (struct nemoart *)data;

	nemocanvas_dispatch_frame(art->canvas);
}

static void nemoart_dispatch_video_done(struct nemoplay *play, void *data)
{
	struct nemoart *art = (struct nemoart *)data;

	art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);

	nemoart_play_video(art, nemofs_dir_get_filepath(art->contents, art->icontents));
}

static int nemoart_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoart *art = (struct nemoart *)nemoaction_get_userdata(action);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		struct actiontap *taps[8];
		int tap0, tap1;
		int ntaps;

		ntaps = nemoaction_get_taps_all(art->action, taps, 8);
		if (ntaps == 1) {
			nemocanvas_move(art->canvas,
					nemoaction_tap_get_serial(taps[0]));
		} else if (ntaps == 2) {
			nemocanvas_pick(art->canvas,
					nemoaction_tap_get_serial(taps[0]),
					nemoaction_tap_get_serial(taps[1]),
					"rotate;scale;translate");
		} else if (ntaps >= 3) {
			nemoaction_get_distant_taps(art->action, taps, ntaps, &tap0, &tap1);

			nemocanvas_pick(art->canvas,
					nemoaction_tap_get_serial(taps[tap0]),
					nemoaction_tap_get_serial(taps[tap1]),
					"rotate;scale;translate");
		}
	}
	if (event & NEMOACTION_TAP_UP_EVENT) {
	}
	if (event & NEMOACTION_TAP_MOTION_EVENT) {
	}

	return 0;
}

static void nemoart_dispatch_bus(void *data, const char *events)
{
	struct nemoart *art = (struct nemoart *)data;
	struct nemoitem *msg;
	struct itemone *one;

	nemofs_dir_clear(art->contents);
	art->icontents = 0;

	msg = nemobus_recv_item(art->bus);
	if (msg == NULL)
		return;

	nemoitem_for_each(one, msg) {
		if (nemoitem_one_has_path_suffix(one, "/play") != 0) {
			const char *path = nemoitem_one_get_attr(one, "url");

			nemofs_dir_insert_file(art->contents, path);
		}
	}

	nemoitem_destroy(msg);

	nemoart_play_video(art, nemofs_dir_get_filepath(art->contents, art->icontents));
}

static int nemoart_play_video(struct nemoart *art, const char *url)
{
	if (art->play != NULL) {
		nemoplay_video_destroy(art->videoback);
		nemoplay_audio_destroy(art->audioback);
		nemoplay_decoder_destroy(art->decoderback);
		nemoplay_destroy(art->play);
	}

	art->play = nemoplay_create();
	if (art->threads != NULL)
		nemoplay_set_video_stropt(art->play, "threads", art->threads);
	nemoplay_load_media(art->play, url);

	art->decoderback = nemoplay_decoder_create(art->play);
	art->audioback = nemoplay_audio_create_by_ao(art->play);
	art->videoback = nemoplay_video_create_by_timer(art->play);
	nemoplay_video_set_texture(art->videoback, 0, art->width, art->height);
	nemoplay_video_set_update(art->videoback, nemoart_dispatch_video_update);
	nemoplay_video_set_done(art->videoback, nemoart_dispatch_video_done);
	nemoplay_video_set_data(art->videoback, art);
	art->shader = nemoplay_video_get_shader(art->videoback);
	nemoplay_shader_set_vertex(art->shader, art->vertex);

	return 0;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,		NULL,		'w' },
		{ "height",				required_argument,		NULL,		'h' },
		{ "fullscreen",		required_argument,		NULL,		'f' },
		{ "content",			required_argument,		NULL,		'c' },
		{ "flip",					required_argument,		NULL,		'l' },
		{ "threads",			required_argument,		NULL,		't' },
		{ "busid",				required_argument,		NULL,		'b' },
		{ 0 }
	};

	struct nemoart *art;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct cookegl *egl;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	char *threads = NULL;
	char *busid = NULL;
	int width = 1920;
	int height = 1080;
	int flip = 0;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:c:l:t:b:", options, NULL)) {
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
				contentpath = strdup(optarg);
				break;

			case 'l':
				flip = strcasecmp(optarg, "on") == 0;
				break;

			case 't':
				threads = strdup(optarg);
				break;

			case 'b':
				busid = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (contentpath == NULL && busid == NULL)
		return 0;

	art = (struct nemoart *)malloc(sizeof(struct nemoart));
	if (art == NULL)
		return -1;
	memset(art, 0, sizeof(struct nemoart));

	art->width = width;
	art->height = height;
	art->vertex = flip == 0 ? NEMOPLAY_SHADER_FLIP_VERTEX : NEMOPLAY_SHADER_FLIP_ROTATE_VERTEX;

	art->threads = threads != NULL ? strdup(threads) : NULL;

	art->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);
	nemotool_connect_egl(tool);

	art->canvas = canvas = nemocanvas_egl_create(tool, width, height);
	nemocanvas_opaque(canvas, 0, 0, width, height);
	nemocanvas_set_nemosurface(canvas, "normal");
	nemocanvas_set_dispatch_resize(canvas, nemoart_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemoart_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemoart_dispatch_canvas_event);
	nemocanvas_set_dispatch_destroy(canvas, nemoart_dispatch_canvas_destroy);
	nemocanvas_set_fullscreen_type(canvas, "pick;pitch");
	nemocanvas_set_state(canvas, "close");
	nemocanvas_set_userdata(canvas, art);

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	art->action = action = nemoaction_create();
	nemoaction_set_tap_callback(action, nemoart_dispatch_tap_event);
	nemoaction_set_userdata(action, art);

	art->egl = egl = nemocook_egl_create(
			NTEGL_DISPLAY(tool),
			NTEGL_CONTEXT(tool),
			NTEGL_CONFIG(tool),
			NTEGL_WINDOW(canvas));
	nemocook_egl_resize(egl, width, height);

	if (busid != NULL) {
		art->bus = nemobus_create();
		nemobus_connect(art->bus, NULL);
		nemobus_advertise(art->bus, "set", busid);

		nemotool_watch_source(tool,
				nemobus_get_socket(art->bus),
				"reh",
				nemoart_dispatch_bus,
				art);

		art->contents = nemofs_dir_create(NULL, 32);
	}

	if (contentpath != NULL) {
		if (os_check_path_is_directory(contentpath) != 0) {
			art->contents = nemofs_dir_create(contentpath, 128);
			nemofs_dir_scan_extension(art->contents, "mp4");
			nemofs_dir_scan_extension(art->contents, "avi");
			nemofs_dir_scan_extension(art->contents, "mov");
			nemofs_dir_scan_extension(art->contents, "ts");
		} else {
			art->contents = nemofs_dir_create(NULL, 32);
			nemofs_dir_insert_file(art->contents, contentpath);
		}

		nemoart_play_video(art, nemofs_dir_get_filepath(art->contents, art->icontents));
	}

	nemotool_run(tool);

	nemoplay_video_destroy(art->videoback);
	nemoplay_audio_destroy(art->audioback);
	nemoplay_decoder_destroy(art->decoderback);

	nemoplay_destroy(art->play);

	if (art->bus != NULL)
		nemobus_destroy(art->bus);

	nemocook_egl_destroy(egl);

	nemoaction_destroy(action);

	nemocanvas_egl_destroy(canvas);

	nemotool_destroy(tool);

	nemofs_dir_destroy(art->contents);

	free(art);

	return 0;
}
