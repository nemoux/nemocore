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
#include <nemotimer.h>
#include <nemojson.h>
#include <nemoitem.h>
#include <nemoaction.h>
#include <nemofs.h>
#include <nemomisc.h>

typedef enum {
	NEMOART_ONESHOT_MODE = 0,
	NEMOART_REPEAT_MODE = 1,
	NEMOART_REPEAT_ALL_MODE = 2,
	NEMOART_LAST_MODE
} NemoArtReplayMode;

struct artone;

typedef struct artone *(*nemoart_one_create_t)(const char *url, int width, int height, int threads, int audio);
typedef void (*nemoart_one_destroy_t)(struct artone *one);
typedef int (*nemoart_one_load_t)(struct artone *one);
typedef void (*nemoart_one_set_integer_t)(struct artone *one, const char *key, int value);
typedef void (*nemoart_one_set_float_t)(struct artone *one, const char *key, float value);
typedef void (*nemoart_one_set_string_t)(struct artone *one, const char *key, const char *value);
typedef void (*nemoart_one_update_t)(struct artone *one);
typedef void (*nemoart_one_done_t)(struct artone *one);
typedef void (*nemoart_one_resize_t)(struct artone *one, int width, int height);
typedef void (*nemoart_one_replay_t)(struct artone *one);
typedef void (*nemoart_one_stop_t)(struct artone *one);
typedef void (*nemoart_one_opaque_t)(struct artone *one, int opaque);
typedef void (*nemoart_one_flip_t)(struct artone *one, int flip);

struct artone {
	nemoart_one_destroy_t destroy;
	nemoart_one_load_t load;
	nemoart_one_set_integer_t set_integer;
	nemoart_one_set_float_t set_float;
	nemoart_one_set_string_t set_string;
	nemoart_one_update_t update;
	nemoart_one_done_t done;
	nemoart_one_resize_t resize;
	nemoart_one_replay_t replay;
	nemoart_one_stop_t stop;

	int width, height;

	int needs_timeout;
};

struct artvideo {
	struct artone one;

	struct nemoplay *play;
	struct playdecoder *decoderback;
	struct playaudio *audioback;
	struct playvideo *videoback;
	struct playshader *shader;

	int threads;
	int audioon;
	double droprate;
};

#define NEMOART_VIDEO(one)			((struct artvideo *)container_of(one, struct artvideo, one))

struct artimage {
	struct cooktex *tex;
};

#define NEMOART_IMAGE(one)			((struct artimage *)container_of(one, struct artimage, one))

struct nemoart {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct nemobus *bus;

	struct nemotimer *alive_timer;
	int alive_timeout;

	int width, height;

	double droprate;

	int threads;
	int flip;
	int audion;
	int opaque;
	int replay;

	struct cookegl *egl;

	struct fsdir *contents;
	int icontents;

	struct artone *one;
};

static void nemoart_one_destroy_video(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (video->audioback != NULL)
		nemoplay_audio_destroy(video->audioback);

	if (video->videoback != NULL)
		nemoplay_video_destroy(video->videoback);

	if (video->decoderback != NULL)
		nemoplay_decoder_destroy(video->decoderback);

	nemoplay_destroy(video->play);

	free(one);
}

static void nemoart_one_resize_video(struct artone *one, int width, int height)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	one->width = width;
	one->height = height;

	nemoplay_video_set_texture(video->videoback, 0, one->width, one->height);
}

static int nemoart_one_load_video(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (video->threads > 0)
		nemoplay_set_video_intopt(video->play, "threads", video->threads);

	nemoplay_load_media(video->play, video->url);

	if (video->audioon == 0)
		nemoplay_revoke_audio(video->play);

	video->decoderback = nemoplay_decoder_create(video->play);
	video->videoback = nemoplay_video_create_by_timer(video->play);
	nemoplay_video_set_drop_rate(video->videoback, video->droprate);
	nemoplay_video_set_texture(video->videoback, 0, one->width, one->height);
	nemoplay_video_set_update(video->videoback, nemoart_dispatch_video_update);
	nemoplay_video_set_done(video->videoback, nemoart_dispatch_video_done);
	nemoplay_video_set_data(video->videoback, one);
	video->shader = nemoplay_video_get_shader(video->videoback);

	if (video->audioon != 0)
		video->audioback = nemoplay_audio_create_by_ao(video->play);

	return 0;
}

static void nemoart_one_set_integer(struct artone *one, const char *key, int value)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (strcmp(key, "threads") == 0)
		video->threads = value;
	else if (strcmp(key, "audio") == 0)
		video->audioon = value;
}

static void nemoart_one_set_float(struct artone *one, const char *key, float value)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (strcmp(key, "droprate") == 0)
		video->droprate = value;
}

static void nemoart_one_set_string(struct artone *one, const char *key, const char *value)
{
	struct artvideo *video = NEMOART_VIDEO(one);
}

static void nemoart_one_update_video(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	nemoplay_shader_dispatch(video->shader);
}

static void nemoart_one_replay_video(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	nemoplay_decoder_seek(video->decoderback, 0.0f);
	nemoplay_decoder_play(video->decoderback);
	nemoplay_video_play(video->videoback);

	if (video->audioback != NULL)
		nemoplay_audio_play(video->audioback);
}

static void nemoart_one_stop_video(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (video->audioback != NULL)
		nemoplay_audio_stop(video->audioback);

	nemoplay_video_stop(video->videoback);
	nemoplay_decoder_stop(video->decoderback);
}

static void nemoart_one_opaque_video(struct artone *one, int opaque)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (opaque == 0)
		nemoplay_shader_set_blend(video->shader, NEMOPLAY_SHADER_SRC_ALPHA_BLEND);
	else
		nemoplay_shader_set_blend(video->shader, NEMOPLAY_SHADER_NONE_BLEND);
}

static void nemoart_one_flip_video(struct artone *one, int flip)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (flip == 0)
		nemoplay_shader_set_polygon(video->shader, NEMOPLAY_SHADER_FLIP_POLYGON);
	else
		nemoplay_shader_set_polygon(video->shader, NEMOPLAY_SHADER_FLIP_ROTATE_POLYGON);
}

static struct artone *nemoart_one_create_video(const char *url, int width, int height, int threads, int audio)
{
	struct artvideo *video;
	struct artone *one;

	video = (struct artvideo *)malloc(sizeof(struct artvideo));
	if (video == NULL)
		return NULL;
	memset(video, 0, sizeof(struct artvideo));

	one = &video->one;

	one->width = width;
	one->height = height;

	one->needs_timeout = 0;

	one->destroy = nemoart_one_destroy_video;
	one->load = nemoart_one_load_video;
	one->set_integer = nemoart_one_set_integer;
	one->set_float = nemoart_one_set_float;
	one->set_string = nemoart_one_set_string;
	one->update = nemoart_one_update_video;
	one->done = nemoart_one_done_video;
	one->resize = nemoart_one_resize_video;
	one->replay = nemoart_one_replay_video;
	one->stop = nemoart_one_stop_video;
	one->opaque = nemoart_one_opaque_video;
	one->flip = nemoart_one_flip_video;

	video->play = nemoplay_create();

	return one;
}

static struct artone *nemoart_one_create_image(const char *url, int width, int height, int threads, int audio)
{
	struct artone *one;

	one = (struct artone *)malloc(sizeof(struct artone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct artone));

	one->width = width;
	one->height = height;

	return one;
}

static int nemoart_one_compare_extension(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

static struct artone *nemoart_one_create(const char *url, int width, int height, int threads, int audio)
{
	static struct extensionelement {
		char extension[32];

		nemoart_one_create_t create;
	} elements[] = {
		{ "avi",					nemoart_one_create_video },
		{ "jpg",					nemoart_one_create_image },
		{ "jpeg",					nemoart_one_create_image },
		{ "mkv",					nemoart_one_create_video },
		{ "mov",					nemoart_one_create_video },
		{ "mp4",					nemoart_one_create_video },
		{ "png",					nemoart_one_create_image },
		{ "ts",						nemoart_one_create_video }
	}, *element;

	const char *extension = os_get_file_extension(url);

	if (extension == NULL)
		return NULL;

	element = (struct extensionelement *)bsearch(extension, elements, sizeof(elements) / sizeof(elements[0]), sizeof(elements[0]), nemoart_one_compare_extension);
	if (element != NULL)
		return element->create(url, width, height, threads, audio);

	return NULL;
}

static inline void nemoart_one_destroy(struct artone *one)
{
	one->destroy(one);
}

static inline void nemoart_one_update(struct artone *one)
{
	one->update(one);
}

static inline void nemoart_one_done(struct artone *one)
{
	one->done(one);
}

static inline void nemoart_one_resize(struct artone *one, int width, int height)
{
	one->resize(one, width, height);
}

static inline void nemoart_one_replay(struct artone *one)
{
	one->replay(one);
}

static inline void nemoart_one_stop(struct artone *one)
{
	one->stop(one);
}

static inline void nemoart_one_opaque(struct artone *one, int opaque)
{
	one->opaque(one, opaque);
}

static inline void nemoart_one_flip(struct artone *one, int flip)
{
	one->flip(one, flip);
}

static void nemoart_dispatch_video_update(struct nemoplay *play, void *data)
{
	struct artone *one = (struct artone *)data;
	struct nemoart *art = one->art;

	nemocanvas_dispatch_frame(art->canvas);
}

static void nemoart_dispatch_video_done(struct nemoplay *play, void *data)
{
	struct artone *one = (struct artone *)data;
	struct nemoart *art = one->art;

	if (art->replay == NEMOART_ONESHOT_MODE || nemofs_dir_get_filecount(art->contents) == 0) {
		nemoart_one_destroy(art->one);
		art->one = NULL;

		nemocanvas_dispatch_frame(art->canvas);
	} else if (art->replay == NEMOART_REPEAT_MODE) {
		nemoart_one_replay(art->one);
	} else {
		int pcontents = art->icontents;

		art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);
		if (art->icontents != pcontents) {
			if (art->one != NULL)
				nemoart_one_destroy(art->one);

			art->one = nemoart_one_create(art,
					nemofs_dir_get_filepath(art->contents, art->icontents),
					art->width, art->height);
		} else {
			nemoart_one_replay(art->one);
		}
	}
}

static void nemoart_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);

	if (width == 0 || height == 0)
		return;

	art->width = width;
	art->height = height;

	nemocanvas_egl_resize(art->canvas, width, height);

	if (art->opaque != 0)
		nemocanvas_opaque(art->canvas, 0, 0, width, height);

	nemocook_egl_resize(art->egl, width, height);

	if (art->one != NULL)
		nemoart_one_resize(art->one, width, height);

	nemocanvas_dispatch_frame(canvas);
}

static void nemoart_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);

	nemocook_egl_make_current(art->egl);

	if (art->one != NULL)
		nemoart_one_update(art->one);
	else
		nemocook_clear_color_buffer(0.0f, 0.0f, 0.0f, 0.0f);

	nemocook_egl_swap_buffers(art->egl);
}

static int nemoart_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemoart *art = (struct nemoart *)nemocanvas_get_userdata(canvas);
	struct actiontap *tap;

	if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		tap = nemoaction_tap_create(art->action);
		nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
		nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
		nemoaction_tap_set_device(tap, nemoevent_get_device(event));
		nemoaction_tap_set_serial(tap, nemoevent_get_serial(event));
		nemoaction_tap_clear(tap,
				nemoevent_get_global_x(event),
				nemoevent_get_global_y(event));
		nemoaction_tap_dispatch_event(art->action, tap, NEMOACTION_TAP_DOWN_EVENT);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		tap = nemoaction_get_tap_by_device(art->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_detach(tap);
			nemoaction_tap_dispatch_event(art->action, tap, NEMOACTION_TAP_UP_EVENT);
			nemoaction_tap_destroy(tap);
		}
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		tap = nemoaction_get_tap_by_device(art->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_trace(tap,
					nemoevent_get_global_x(event),
					nemoevent_get_global_y(event));
			nemoaction_tap_dispatch_event(art->action, tap, NEMOACTION_TAP_MOTION_EVENT);
		}
	}

	return 0;
}

static int nemoart_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	nemotool_exit(nemocanvas_get_tool(canvas));

	return 1;
}

static int nemoart_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoart *art = (struct nemoart *)nemoaction_get_userdata(action);

	if ((event & NEMOACTION_TAP_DOWN_EVENT) || (event & NEMOACTION_TAP_UP_EVENT)) {
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

	return 0;
}

static void nemoart_dispatch_alive_timer(struct nemotimer *timer, void *data)
{
	struct nemoart *art = (struct nemoart *)data;

	nemotool_client_alive(art->tool, art->alive_timeout);

	nemotimer_set_timeout(art->alive_timer, art->alive_timeout / 2);
}

static void nemoart_dispatch_bus(void *data, const char *events)
{
	struct nemoart *art = (struct nemoart *)data;
	struct nemojson *json;
	struct nemoitem *msg;
	struct itemone *one;
	char buffer[4096];
	int length;
	int i;

	length = nemobus_recv(art->bus, buffer, sizeof(buffer));
	if (length <= 0)
		return;

	json = nemojson_create(buffer, length);
	nemojson_update(json);

	for (i = 0; i < nemojson_get_object_count(json); i++) {
		msg = nemoitem_create();
		nemoitem_load_json(msg, "/nemoart", nemojson_get_object(json, i));

		nemoitem_for_each(one, msg) {
			if (nemoitem_one_has_path_suffix(one, "/play") != 0) {
				const char *url = nemoitem_one_get_attr(one, "url");

				if (url != NULL) {
					const char *mode = nemoitem_one_get_sattr(one, "mode", "repeat_all");

					if (strcmp(url, "@next") == 0)
						art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);
					else if (strcmp(url, "@prev") == 0)
						art->icontents = (art->icontents + nemofs_dir_get_filecount(art->contents) - 1) % nemofs_dir_get_filecount(art->contents);
					else if (strcmp(url, "@random") == 0)
						art->icontents = random_get_int(0, nemofs_dir_get_filecount(art->contents) - 1);
					else
						art->icontents = nemofs_dir_insert_file(art->contents, NULL, url);

					if (strcmp(mode, "oneshot") == 0)
						art->replay = NEMOART_ONESHOT_MODE;
					else if (strcmp(mode, "repeat") == 0)
						art->replay = NEMOART_REPEAT_MODE;
					else
						art->replay = NEMOART_REPEAT_ALL_MODE;

					if (art->one != NULL)
						nemoart_one_destroy(art->one);

					art->one = nemoart_one_create(art,
							nemofs_dir_get_filepath(art->contents, art->icontents),
							art->width, art->height);
				}
			} else if (nemoitem_one_has_path_suffix(one, "/clear") != 0) {
				nemofs_dir_clear(art->contents);

				art->icontents = 0;
			} else if (nemoitem_one_has_path_suffix(one, "/append") != 0) {
				const char *url = nemoitem_one_get_attr(one, "url");

				if (url != NULL)
					nemofs_dir_insert_file(art->contents, NULL, url);
			}
		}

		nemoitem_destroy(msg);
	}

	nemojson_destroy(json);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,		NULL,		'w' },
		{ "height",				required_argument,		NULL,		'h' },
		{ "fullscreen",		required_argument,		NULL,		'f' },
		{ "content",			required_argument,		NULL,		'c' },
		{ "replay",				required_argument,		NULL,		'r' },
		{ "maximum",			required_argument,		NULL,		'm' },
		{ "droprate",			required_argument,		NULL,		'd' },
		{ "pick",					required_argument,		NULL,		'p' },
		{ "flip",					required_argument,		NULL,		'l' },
		{ "opaque",				required_argument,		NULL,		'q' },
		{ "layer",				required_argument,		NULL,		'y' },
		{ "threads",			required_argument,		NULL,		't' },
		{ "audio",				required_argument,		NULL,		'a' },
		{ "busid",				required_argument,		NULL,		'b' },
		{ "alive",				required_argument,		NULL,		'e' },
		{ 0 }
	};

	struct nemoart *art;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct cookegl *egl;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	char *busid = NULL;
	char *layer = NULL;
	double droprate = 2.0f;
	int width = 1920;
	int height = 1080;
	int replay = NEMOART_REPEAT_ALL_MODE;
	int maximum = 128;
	int threads = 0;
	int audion = 0;
	int pick = 1;
	int flip = 0;
	int opaque = 1;
	int alive = 0;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:c:r:m:d:p:l:q:y:t:a:b:e:", options, NULL)) {
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

			case 'r':
				if (strcmp(optarg, "oneshot") == 0)
					replay = NEMOART_ONESHOT_MODE;
				else if (strcmp(optarg, "repeat") == 0)
					replay = NEMOART_REPEAT_MODE;
				break;

			case 'm':
				maximum = strtoul(optarg, NULL, 10);
				break;

			case 'd':
				droprate = strtod(optarg, NULL);
				break;

			case 'p':
				pick = strcasecmp(optarg, "on") == 0;
				break;

			case 'l':
				flip = strcasecmp(optarg, "on") == 0;
				break;

			case 'q':
				opaque = strcasecmp(optarg, "on") == 0;
				break;

			case 't':
				threads = strtoul(optarg, NULL, 10);
				break;

			case 'y':
				layer = strdup(optarg);
				break;

			case 'a':
				audion = strcasecmp(optarg, "on") == 0;
				break;

			case 'b':
				busid = strdup(optarg);
				break;

			case 'e':
				alive = strtoul(optarg, NULL, 10);
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
	art->droprate = droprate;
	art->threads = threads;
	art->flip = flip;
	art->audion = audion;
	art->opaque = opaque;
	art->replay = replay;
	art->alive_timeout = alive;

	art->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);
	nemotool_connect_egl(tool);

	art->alive_timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemoart_dispatch_alive_timer);
	nemotimer_set_userdata(timer, art);
	nemotimer_set_timeout(timer, art->alive_timeout / 2);

	art->canvas = canvas = nemocanvas_egl_create(tool, width, height);
	nemocanvas_set_nemosurface(canvas, "normal");
	nemocanvas_set_dispatch_resize(canvas, nemoart_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemoart_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemoart_dispatch_canvas_event);
	nemocanvas_set_dispatch_destroy(canvas, nemoart_dispatch_canvas_destroy);
	nemocanvas_set_fullscreen_type(canvas, "pick;pitch");
	nemocanvas_set_state(canvas, "close");
	nemocanvas_set_userdata(canvas, art);

	if (pick == 0)
		nemocanvas_put_state(canvas, "pick");

	if (opaque != 0)
		nemocanvas_opaque(canvas, 0, 0, width, height);

	if (layer != NULL)
		nemocanvas_set_layer(canvas, layer);

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

	art->contents = nemofs_dir_create(maximum);

	if (busid != NULL) {
		art->bus = nemobus_create();
		nemobus_connect(art->bus, NULL);
		nemobus_advertise(art->bus, "set", busid);

		nemotool_watch_source(tool,
				nemobus_get_socket(art->bus),
				"reh",
				nemoart_dispatch_bus,
				art);
	}

	if (contentpath != NULL) {
		if (os_check_is_directory(contentpath) != 0)
			nemofs_dir_scan_extensions(art->contents, contentpath, 5, "mp4", "avi", "mov", "mkv", "ts");
		else
			nemofs_dir_insert_file(art->contents, NULL, contentpath);

		art->one = nemoart_one_create(art,
				nemofs_dir_get_filepath(art->contents, art->icontents),
				art->width, art->height);
	}

	nemotool_run(tool);

	if (art->one != NULL)
		nemoart_one_destroy(art->one);

	if (art->bus != NULL)
		nemobus_destroy(art->bus);

	nemocook_egl_destroy(egl);

	nemoaction_destroy(action);

	nemocanvas_egl_destroy(canvas);

	nemotimer_destroy(timer);

	nemotool_destroy(tool);

	nemofs_dir_destroy(art->contents);

	free(art);

	return 0;
}
