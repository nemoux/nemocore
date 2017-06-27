#define _GNU_SOURCE
#define __USE_GNU
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
#include <nemostring.h>
#include <nemonoty.h>
#include <nemomisc.h>

typedef enum {
	NEMOART_ONE_NONE_TYPE = 0,
	NEMOART_ONE_VIDEO_TYPE = 1,
	NEMOART_ONE_IMAGE_TYPE = 2,
	NEMOART_ONE_LAST_TYPE
} NemoArtOneType;

typedef enum {
	NEMOART_ONESHOT_MODE = 0,
	NEMOART_REPEAT_MODE = 1,
	NEMOART_REPEAT_ALL_MODE = 2,
	NEMOART_LAST_MODE
} NemoArtReplayMode;

struct artone;

typedef struct artone *(*nemoart_one_create_t)(const char *url, int width, int height);
typedef void (*nemoart_one_destroy_t)(struct artone *one);
typedef int (*nemoart_one_load_t)(struct artone *one);
typedef void (*nemoart_one_stop_t)(struct artone *one);
typedef void (*nemoart_one_update_t)(struct artone *one);
typedef void (*nemoart_one_resize_t)(struct artone *one, int width, int height);
typedef void (*nemoart_one_replay_t)(struct artone *one);
typedef void (*nemoart_one_set_integer_t)(struct artone *one, const char *key, int value);
typedef void (*nemoart_one_set_float_t)(struct artone *one, const char *key, float value);
typedef void (*nemoart_one_set_string_t)(struct artone *one, const char *key, const char *value);
typedef void (*nemoart_one_set_pointer_t)(struct artone *one, const char *key, void *value);

struct artone {
	nemoart_one_destroy_t destroy;
	nemoart_one_load_t load;
	nemoart_one_stop_t stop;
	nemoart_one_update_t update;
	nemoart_one_resize_t resize;
	nemoart_one_replay_t replay;
	nemoart_one_set_integer_t set_integer;
	nemoart_one_set_float_t set_float;
	nemoart_one_set_string_t set_string;
	nemoart_one_set_pointer_t set_pointer;

	char *url;
	int type;
	int width, height;

	struct nemonoty *update_noty;
	struct nemonoty *done_noty;
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
	int opaque;
	int flip;
	double droprate;
};

#define NEMOART_VIDEO(one)			((struct artvideo *)container_of(one, struct artvideo, one))

struct artimage {
	struct artone one;

	struct cooktex *tex;
	struct cookshader *shader;

	int flip;
};

#define NEMOART_IMAGE(one)			((struct artimage *)container_of(one, struct artimage, one))

struct nemoart {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct nemobus *bus;

	struct nemotimer *alive_timer;
	int alive_timeout;

	struct nemotimer *tween_timer;
	int tween_timeout;

	int width, height;

	double droprate;

	int threads;
	int audioon;
	int flip;
	int opaque;
	int replay;

	struct cookegl *egl;
	struct cookshader *shader;

	struct fsdir *contents;
	int icontents;

	struct artone *one;
};

static void nemoart_video_dispatch_update(struct nemoplay *play, void *data)
{
	struct artone *one = (struct artone *)data;

	nemonoty_dispatch(one->update_noty, one);
}

static void nemoart_video_dispatch_done(struct nemoplay *play, void *data)
{
	struct artone *one = (struct artone *)data;

	nemonoty_dispatch(one->done_noty, one);
}

static void nemoart_video_destroy(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (video->audioback != NULL)
		nemoplay_audio_destroy(video->audioback);

	if (video->videoback != NULL)
		nemoplay_video_destroy(video->videoback);

	if (video->decoderback != NULL)
		nemoplay_decoder_destroy(video->decoderback);

	nemoplay_destroy(video->play);

	if (one->update_noty != NULL)
		nemonoty_destroy(one->update_noty);
	if (one->done_noty != NULL)
		nemonoty_destroy(one->done_noty);

	if (one->url != NULL)
		free(one->url);

	free(video);
}

static int nemoart_video_load(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (video->threads > 0)
		nemoplay_set_video_intopt(video->play, "threads", video->threads);

	if (nemoplay_load_media(video->play, one->url) < 0)
		return -1;

	video->decoderback = nemoplay_decoder_create(video->play);
	video->videoback = nemoplay_video_create_by_timer(video->play);
	nemoplay_video_set_drop_rate(video->videoback, video->droprate);
	nemoplay_video_set_texture(video->videoback, 0, one->width, one->height);
	nemoplay_video_set_update(video->videoback, nemoart_video_dispatch_update);
	nemoplay_video_set_done(video->videoback, nemoart_video_dispatch_done);
	nemoplay_video_set_data(video->videoback, one);
	video->shader = nemoplay_video_get_shader(video->videoback);

	if (video->audioon != 0)
		video->audioback = nemoplay_audio_create_by_ao(video->play);
	else
		nemoplay_revoke_audio(video->play);

	if (video->opaque != 0)
		nemoplay_shader_set_blend(video->shader, NEMOPLAY_SHADER_NONE_BLEND);
	else
		nemoplay_shader_set_blend(video->shader, NEMOPLAY_SHADER_SRC_ALPHA_BLEND);

	if (video->flip != 0)
		nemoplay_shader_set_polygon(video->shader, NEMOPLAY_SHADER_FLIP_ROTATE_POLYGON);
	else
		nemoplay_shader_set_polygon(video->shader, NEMOPLAY_SHADER_FLIP_POLYGON);

	return 0;
}

static void nemoart_video_stop(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (video->audioback != NULL)
		nemoplay_audio_stop(video->audioback);

	nemoplay_video_stop(video->videoback);
	nemoplay_decoder_stop(video->decoderback);
}

static void nemoart_video_update(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	nemoplay_shader_dispatch(video->shader);
}

static void nemoart_video_resize(struct artone *one, int width, int height)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	one->width = width;
	one->height = height;

	nemoplay_video_set_texture(video->videoback, 0, one->width, one->height);
}

static void nemoart_video_replay(struct artone *one)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	nemoplay_decoder_seek(video->decoderback, 0.0f);
	nemoplay_decoder_play(video->decoderback);
	nemoplay_video_play(video->videoback);

	if (video->audioback != NULL)
		nemoplay_audio_play(video->audioback);
}

static void nemoart_video_set_integer(struct artone *one, const char *key, int value)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (strcmp(key, "threads") == 0) {
		video->threads = value;
	} else if (strcmp(key, "audio") == 0) {
		video->audioon = value;
	} else if (strcmp(key, "opaque") == 0) {
		video->opaque = value;

		if (video->shader != NULL) {
			if (video->opaque != 0)
				nemoplay_shader_set_blend(video->shader, NEMOPLAY_SHADER_NONE_BLEND);
			else
				nemoplay_shader_set_blend(video->shader, NEMOPLAY_SHADER_SRC_ALPHA_BLEND);
		}
	} else if (strcmp(key, "flip") == 0) {
		video->flip = value;

		if (video->shader != NULL) {
			if (video->flip != 0)
				nemoplay_shader_set_polygon(video->shader, NEMOPLAY_SHADER_FLIP_ROTATE_POLYGON);
			else
				nemoplay_shader_set_polygon(video->shader, NEMOPLAY_SHADER_FLIP_POLYGON);
		}
	}
}

static void nemoart_video_set_float(struct artone *one, const char *key, float value)
{
	struct artvideo *video = NEMOART_VIDEO(one);

	if (strcmp(key, "droprate") == 0)
		video->droprate = value;
}

static void nemoart_video_set_string(struct artone *one, const char *key, const char *value)
{
	struct artvideo *video = NEMOART_VIDEO(one);
}

static void nemoart_video_set_pointer(struct artone *one, const char *key, void *value)
{
	struct artvideo *video = NEMOART_VIDEO(one);
}

static struct artone *nemoart_video_create(const char *url, int width, int height)
{
	struct artvideo *video;
	struct artone *one;

	video = (struct artvideo *)malloc(sizeof(struct artvideo));
	if (video == NULL)
		return NULL;
	memset(video, 0, sizeof(struct artvideo));

	one = &video->one;

	one->url = strdup(url);
	one->type = NEMOART_ONE_VIDEO_TYPE;
	one->width = width;
	one->height = height;

	one->destroy = nemoart_video_destroy;
	one->load = nemoart_video_load;
	one->stop = nemoart_video_stop;
	one->update = nemoart_video_update;
	one->resize = nemoart_video_resize;
	one->replay = nemoart_video_replay;
	one->set_integer = nemoart_video_set_integer;
	one->set_float = nemoart_video_set_float;
	one->set_string = nemoart_video_set_string;
	one->set_pointer = nemoart_video_set_pointer;

	one->update_noty = nemonoty_create();
	one->done_noty = nemonoty_create();

	video->play = nemoplay_create();

	video->threads = 0;
	video->audioon = 1;
	video->droprate = 2.0f;

	return one;
}

static void nemoart_image_destroy(struct artone *one)
{
	struct artimage *image = NEMOART_IMAGE(one);

	if (image->tex != NULL)
		nemocook_texture_destroy(image->tex);

	if (one->update_noty != NULL)
		nemonoty_destroy(one->update_noty);
	if (one->done_noty != NULL)
		nemonoty_destroy(one->done_noty);

	if (one->url != NULL)
		free(one->url);

	free(image);
}

static int nemoart_image_load(struct artone *one)
{
	struct artimage *image = NEMOART_IMAGE(one);

	if (nemocook_texture_load_image(image->tex, one->url) < 0)
		return -1;

	nemonoty_dispatch(one->update_noty, one);

	return 0;
}

static void nemoart_image_stop(struct artone *one)
{
	struct artimage *image = NEMOART_IMAGE(one);
}

static void nemoart_image_update(struct artone *one)
{
	struct artimage *image = NEMOART_IMAGE(one);

	if (image->shader != NULL) {
		nemocook_shader_use_program(image->shader);

		if (image->flip != 0) {
			nemocook_draw_texture(
					nemocook_texture_get(image->tex),
					-1.0f, 1.0f, 1.0f, -1.0f);
		} else {
			nemocook_draw_texture(
					nemocook_texture_get(image->tex),
					-1.0f, -1.0f, 1.0f, 1.0f);
		}
	}
}

static void nemoart_image_resize(struct artone *one, int width, int height)
{
	struct artimage *image = NEMOART_IMAGE(one);

	one->width = width;
	one->height = height;

	nemocook_texture_resize(image->tex, one->width, one->height);

	nemocook_texture_load_image(image->tex, one->url);
}

static void nemoart_image_replay(struct artone *one)
{
	struct artimage *image = NEMOART_IMAGE(one);
}

static void nemoart_image_set_integer(struct artone *one, const char *key, int value)
{
	struct artimage *image = NEMOART_IMAGE(one);

	if (strcmp(key, "flip") == 0) {
		image->flip = value;
	}
}

static void nemoart_image_set_float(struct artone *one, const char *key, float value)
{
	struct artimage *image = NEMOART_IMAGE(one);
}

static void nemoart_image_set_string(struct artone *one, const char *key, const char *value)
{
	struct artimage *image = NEMOART_IMAGE(one);
}

static void nemoart_image_set_pointer(struct artone *one, const char *key, void *value)
{
	struct artimage *image = NEMOART_IMAGE(one);

	if (strcmp(key, "shader") == 0) {
		image->shader = (struct cookshader *)value;
	}
}

static struct artone *nemoart_image_create(const char *url, int width, int height)
{
	struct artimage *image;
	struct artone *one;

	image = (struct artimage *)malloc(sizeof(struct artimage));
	if (image == NULL)
		return NULL;
	memset(image, 0, sizeof(struct artimage));

	one = &image->one;

	one->url = strdup(url);
	one->type = NEMOART_ONE_IMAGE_TYPE;
	one->width = width;
	one->height = height;

	one->destroy = nemoart_image_destroy;
	one->load = nemoart_image_load;
	one->stop = nemoart_image_stop;
	one->update = nemoart_image_update;
	one->resize = nemoart_image_resize;
	one->replay = nemoart_image_replay;
	one->set_integer = nemoart_image_set_integer;
	one->set_float = nemoart_image_set_float;
	one->set_string = nemoart_image_set_string;
	one->set_pointer = nemoart_image_set_pointer;

	one->update_noty = nemonoty_create();
	one->done_noty = nemonoty_create();

	image->tex = nemocook_texture_create();
	nemocook_texture_assign(image->tex, NEMOCOOK_TEXTURE_BGRA_FORMAT, width, height);

	return one;
}

static int nemoart_one_compare_extension(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

static struct artone *nemoart_one_create(const char *url, int width, int height)
{
	static struct extensionelement {
		char extension[32];

		nemoart_one_create_t create;
	} elements[] = {
		{ ".avi",					nemoart_video_create },
		{ ".jpeg",				nemoart_image_create },
		{ ".jpg",					nemoart_image_create },
		{ ".mkv",					nemoart_video_create },
		{ ".mov",					nemoart_video_create },
		{ ".mp4",					nemoart_video_create },
		{ ".png",					nemoart_image_create },
		{ ".svg",					nemoart_image_create },
		{ ".ts",					nemoart_video_create },
		{ ".wmv",					nemoart_video_create }
	}, *element;

	const char *extension = strrchr(url, '.');

	if (extension == NULL)
		return NULL;

	element = (struct extensionelement *)bsearch(extension, elements, sizeof(elements) / sizeof(elements[0]), sizeof(elements[0]), nemoart_one_compare_extension);
	if (element != NULL)
		return element->create(url, width, height);

	return NULL;
}

static inline int nemoart_one_is_type(struct artone *one, int type)
{
	return one->type == type;
}

static inline void nemoart_one_destroy(struct artone *one)
{
	one->destroy(one);
}

static inline int nemoart_one_load(struct artone *one)
{
	return one->load(one);
}

static inline void nemoart_one_set_integer(struct artone *one, const char *key, int value)
{
	one->set_integer(one, key, value);
}

static inline void nemoart_one_set_float(struct artone *one, const char *key, float value)
{
	one->set_float(one, key, value);
}

static inline void nemoart_one_set_string(struct artone *one, const char *key, const char *value)
{
	one->set_string(one, key, value);
}

static inline void nemoart_one_set_pointer(struct artone *one, const char *key, void *value)
{
	one->set_pointer(one, key, value);
}

static inline void nemoart_one_update(struct artone *one)
{
	one->update(one);
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

static void nemoart_one_set_update_callback(struct artone *one, nemonoty_dispatch_t dispatch, void *data)
{
	nemonoty_attach(one->update_noty, dispatch, data);
}

static void nemoart_one_set_done_callback(struct artone *one, nemonoty_dispatch_t dispatch, void *data)
{
	nemonoty_attach(one->done_noty, dispatch, data);
}

static int nemoart_dispatch_one_update(void *data, void *event)
{
	struct nemoart *art = (struct nemoart *)data;

	nemocanvas_dispatch_frame(art->canvas);

	return 0;
}

static int nemoart_dispatch_one_done(void *data, void *event)
{
	struct nemoart *art = (struct nemoart *)data;

	if (art->replay == NEMOART_ONESHOT_MODE || nemofs_dir_get_filecount(art->contents) == 0) {
		if (art->one != NULL) {
			nemoart_one_destroy(art->one);
			art->one = NULL;
		}

		nemocanvas_dispatch_frame(art->canvas);
	} else if (art->replay == NEMOART_REPEAT_MODE) {
		if (art->one != NULL)
			nemoart_one_replay(art->one);
	} else {
		int pcontents = art->icontents;

		art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);
		if (art->icontents != pcontents) {
			if (art->one != NULL)
				nemoart_one_destroy(art->one);

			art->one = nemoart_one_create(
					nemofs_dir_get_filepath(art->contents, art->icontents),
					art->width, art->height);
			if (art->one != NULL) {
				if (nemoart_one_is_type(art->one, NEMOART_ONE_VIDEO_TYPE) != 0) {
					nemoart_one_set_integer(art->one, "threads", art->threads);
					nemoart_one_set_integer(art->one, "audio", art->audioon);
					nemoart_one_set_float(art->one, "droprate", art->droprate);
					nemotimer_set_timeout(art->tween_timer, 0);
				} else if (nemoart_one_is_type(art->one, NEMOART_ONE_IMAGE_TYPE) != 0) {
					nemoart_one_set_pointer(art->one, "shader", art->shader);
					nemotimer_set_timeout(art->tween_timer, art->tween_timeout);
				}
				nemoart_one_set_integer(art->one, "opaque", art->opaque);
				nemoart_one_set_integer(art->one, "flip", art->flip);
				nemoart_one_set_update_callback(art->one, nemoart_dispatch_one_update, art);
				nemoart_one_set_done_callback(art->one, nemoart_dispatch_one_done, art);
				nemoart_one_load(art->one);
			}
		} else if (art->one != NULL) {
			nemoart_one_replay(art->one);
		}
	}

	return 1;
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

static void nemoart_dispatch_tween_timer(struct nemotimer *timer, void *data)
{
	struct nemoart *art = (struct nemoart *)data;

	if (art->replay == NEMOART_ONESHOT_MODE || nemofs_dir_get_filecount(art->contents) == 0) {
		if (art->one != NULL) {
			nemoart_one_destroy(art->one);
			art->one = NULL;
		}

		nemocanvas_dispatch_frame(art->canvas);
	} else if (art->replay == NEMOART_REPEAT_MODE) {
		if (art->one != NULL)
			nemoart_one_replay(art->one);
	} else {
		int pcontents = art->icontents;

		art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);
		if (art->icontents != pcontents) {
			if (art->one != NULL)
				nemoart_one_destroy(art->one);

			art->one = nemoart_one_create(
					nemofs_dir_get_filepath(art->contents, art->icontents),
					art->width, art->height);
			if (art->one != NULL) {
				if (nemoart_one_is_type(art->one, NEMOART_ONE_VIDEO_TYPE) != 0) {
					nemoart_one_set_integer(art->one, "threads", art->threads);
					nemoart_one_set_integer(art->one, "audio", art->audioon);
					nemoart_one_set_float(art->one, "droprate", art->droprate);
					nemotimer_set_timeout(art->tween_timer, 0);
				} else if (nemoart_one_is_type(art->one, NEMOART_ONE_IMAGE_TYPE) != 0) {
					nemoart_one_set_pointer(art->one, "shader", art->shader);
					nemotimer_set_timeout(art->tween_timer, art->tween_timeout);
				}
				nemoart_one_set_integer(art->one, "opaque", art->opaque);
				nemoart_one_set_integer(art->one, "flip", art->flip);
				nemoart_one_set_update_callback(art->one, nemoart_dispatch_one_update, art);
				nemoart_one_set_done_callback(art->one, nemoart_dispatch_one_done, art);
				nemoart_one_load(art->one);
			}
		} else if (art->one != NULL) {
			nemoart_one_replay(art->one);
		}
	}
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

	json = nemojson_create_string(buffer, length);
	nemojson_update(json);

	for (i = 0; i < nemojson_get_count(json); i++) {
		msg = nemoitem_create();
		nemojson_object_load_item(nemojson_get_object(json, i), msg, "/nemoart");

		nemoitem_for_each(one, msg) {
			if (nemoitem_one_has_path_suffix(one, "/play") != 0) {
				const char *url = nemoitem_one_get_attr(one, "url");

				if (url != NULL) {
					const char *mode = nemoitem_one_get_sattr(one, "mode", "repeat_all");

					if (strcmp(url, "@next") == 0) {
						art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);
					} else if (strcmp(url, "@prev") == 0) {
						art->icontents = (art->icontents + nemofs_dir_get_filecount(art->contents) - 1) % nemofs_dir_get_filecount(art->contents);
					} else if (strcmp(url, "@random") == 0) {
						art->icontents = random_get_integer(0, nemofs_dir_get_filecount(art->contents) - 1);
					} else {
						char *path;

						path = nemostring_replace(url, "@contents", env_get_string("NEMO_CONTENTS_PATH", "/opt/contents"));
						if (path != NULL) {
							if (os_file_is_directory(path) != 0)
								nemofs_dir_scan_extensions(art->contents, path, 10, "mp4", "avi", "mov", "mkv", "ts", "wmv", "png", "jpg", "jpeg", "svg");
							else if (os_file_is_exist(path) != 0)
								nemofs_dir_insert_file(art->contents, NULL, path);

							free(path);
						} else {
							if (os_file_is_directory(url) != 0)
								nemofs_dir_scan_extensions(art->contents, url, 10, "mp4", "avi", "mov", "mkv", "ts", "wmv", "png", "jpg", "jpeg", "svg");
							else if (os_file_is_exist(url) != 0)
								nemofs_dir_insert_file(art->contents, NULL, url);
						}

						art->icontents = (art->icontents + 1) % nemofs_dir_get_filecount(art->contents);
					}

					if (nemofs_dir_get_filecount(art->contents) > 0) {
						if (strcmp(mode, "oneshot") == 0)
							art->replay = NEMOART_ONESHOT_MODE;
						else if (strcmp(mode, "repeat") == 0)
							art->replay = NEMOART_REPEAT_MODE;
						else
							art->replay = NEMOART_REPEAT_ALL_MODE;

						if (art->one != NULL)
							nemoart_one_destroy(art->one);

						art->one = nemoart_one_create(
								nemofs_dir_get_filepath(art->contents, art->icontents),
								art->width, art->height);
						if (art->one != NULL) {
							if (nemoart_one_is_type(art->one, NEMOART_ONE_VIDEO_TYPE) != 0) {
								nemoart_one_set_integer(art->one, "threads", art->threads);
								nemoart_one_set_integer(art->one, "audio", art->audioon);
								nemoart_one_set_float(art->one, "droprate", art->droprate);
								nemotimer_set_timeout(art->tween_timer, 0);
							} else if (nemoart_one_is_type(art->one, NEMOART_ONE_IMAGE_TYPE) != 0) {
								nemoart_one_set_pointer(art->one, "shader", art->shader);
								nemotimer_set_timeout(art->tween_timer, art->tween_timeout);
							}
							nemoart_one_set_integer(art->one, "opaque", art->opaque);
							nemoart_one_set_integer(art->one, "flip", art->flip);
							nemoart_one_set_update_callback(art->one, nemoart_dispatch_one_update, art);
							nemoart_one_set_done_callback(art->one, nemoart_dispatch_one_done, art);
							nemoart_one_load(art->one);
						}
					}
				}
			} else if (nemoitem_one_has_path_suffix(one, "/clear") != 0) {
				nemofs_dir_clear(art->contents);

				art->icontents = 0;
			} else if (nemoitem_one_has_path_suffix(one, "/append") != 0) {
				const char *url = nemoitem_one_get_attr(one, "url");

				if (url != NULL) {
					if (os_file_is_directory(url) != 0)
						nemofs_dir_scan_extensions(art->contents, url, 10, "mp4", "avi", "mov", "mkv", "ts", "wmv", "png", "jpg", "jpeg", "svg");
					else if (os_file_is_exist(url) != 0)
						nemofs_dir_insert_file(art->contents, NULL, url);
				}
			}
		}

		nemoitem_destroy(msg);
	}

	nemojson_destroy(json);
}

int main(int argc, char *argv[])
{
	static const char texture_vertex_shader[] =
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";

	static const char texture_fragment_shader[] =
		"precision mediump float;\n"
		"varying vec2 vtexcoord;\n"
		"uniform sampler2D tex;\n"
		"void main()\n"
		"{\n"
		"  vec4 c = texture2D(tex, vtexcoord);\n"
		"  gl_FragColor = vec4(c.b, c.g, c.r, c.a);\n"
		"}\n";

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
		{ "tween",				required_argument,		NULL,		'n' },
		{ 0 }
	};

	struct nemoart *art;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct cookegl *egl;
	struct cookshader *shader;
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
	int audioon = 0;
	int pick = 1;
	int flip = 0;
	int opaque = 1;
	int alive = 0;
	int tween = 5000;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:c:r:m:d:p:l:q:y:t:a:b:e:n:", options, NULL)) {
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
				audioon = strcasecmp(optarg, "on") == 0;
				break;

			case 'b':
				busid = strdup(optarg);
				break;

			case 'e':
				alive = strtoul(optarg, NULL, 10);
				break;

			case 'n':
				tween = strtoul(optarg, NULL, 10);
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
	art->audioon = audioon;
	art->flip = flip;
	art->opaque = opaque;
	art->replay = replay;
	art->alive_timeout = alive;
	art->tween_timeout = tween;

	art->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);
	nemotool_connect_egl(tool, 1, 4);

	art->alive_timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemoart_dispatch_alive_timer);
	nemotimer_set_userdata(timer, art);
	nemotimer_set_timeout(timer, art->alive_timeout / 2);

	art->tween_timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemoart_dispatch_tween_timer);
	nemotimer_set_userdata(timer, art);

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

	art->shader = shader = nemocook_shader_create();
	nemocook_shader_set_program(shader, texture_vertex_shader, texture_fragment_shader);
	nemocook_shader_set_attrib(shader, 0, "position", 2);
	nemocook_shader_set_attrib(shader, 1, "texcoord", 2);

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
		char *replacepath;

		replacepath = nemostring_replace(contentpath, "@contents", env_get_string("NEMO_CONTENTS_PATH", "/opt/contents"));
		if (replacepath != NULL) {
			free(contentpath);

			contentpath = replacepath;
		}

		if (os_file_is_directory(contentpath) != 0)
			nemofs_dir_scan_extensions(art->contents, contentpath, 10, "mp4", "avi", "mov", "mkv", "ts", "wmv", "png", "jpg", "jpeg", "svg");
		else if (os_file_is_exist(contentpath) != 0)
			nemofs_dir_insert_file(art->contents, NULL, contentpath);

		if (nemofs_dir_get_filecount(art->contents) > 0) {
			art->one = nemoart_one_create(
					nemofs_dir_get_filepath(art->contents, art->icontents),
					art->width, art->height);
			if (art->one != NULL) {
				if (nemoart_one_is_type(art->one, NEMOART_ONE_VIDEO_TYPE) != 0) {
					nemoart_one_set_integer(art->one, "threads", art->threads);
					nemoart_one_set_integer(art->one, "audio", art->audioon);
					nemoart_one_set_float(art->one, "droprate", art->droprate);
					nemotimer_set_timeout(art->tween_timer, 0);
				} else if (nemoart_one_is_type(art->one, NEMOART_ONE_IMAGE_TYPE) != 0) {
					nemoart_one_set_pointer(art->one, "shader", art->shader);
					nemotimer_set_timeout(art->tween_timer, art->tween_timeout);
				}
				nemoart_one_set_integer(art->one, "opaque", art->opaque);
				nemoart_one_set_integer(art->one, "flip", art->flip);
				nemoart_one_set_update_callback(art->one, nemoart_dispatch_one_update, art);
				nemoart_one_set_done_callback(art->one, nemoart_dispatch_one_done, art);
				nemoart_one_load(art->one);
			}
		}
	}

	nemotool_run(tool);

	if (art->one != NULL)
		nemoart_one_destroy(art->one);

	if (art->bus != NULL)
		nemobus_destroy(art->bus);

	nemocook_shader_destroy(shader);
	nemocook_egl_destroy(egl);

	nemoaction_destroy(action);

	nemocanvas_egl_destroy(canvas);

	nemotimer_destroy(timer);

	nemotool_destroy(tool);

	nemofs_dir_destroy(art->contents);

	free(art);

	return 0;
}
