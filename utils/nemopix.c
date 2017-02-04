#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemocanvas.h>

#include <nemomotz.h>
#include <motzobject.h>
#include <nemobus.h>
#include <nemojson.h>
#include <nemoitem.h>
#include <nemoaction.h>
#include <nemomisc.h>

struct nemopix {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct nemobus *bus;
	struct nemomotz *motz;

	struct nemotimer *alive_timer;
	int alive_timeout;

	int width, height;
	int opaque;
};

static void nemopix_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemopix *pix = (struct nemopix *)nemocanvas_get_userdata(canvas);

	if (width == 0 || height == 0)
		return;

	pix->width = width;
	pix->height = height;

	nemocanvas_set_size(canvas, width, height);

	if (pix->opaque != 0)
		nemocanvas_opaque(canvas, 0, 0, width, height);

	nemocanvas_dispatch_frame(canvas);
}

static void nemopix_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemopix *pix = (struct nemopix *)nemocanvas_get_userdata(canvas);
	pixman_image_t *framebuffer;

	nemomotz_update(pix->motz);

	nemocanvas_buffer(canvas);

	framebuffer = nemocanvas_get_pixman_image(canvas);
	if (framebuffer != NULL) {
		nemomotz_attach_buffer(pix->motz,
				pixman_image_get_data(framebuffer),
				pixman_image_get_width(framebuffer),
				pixman_image_get_height(framebuffer));
		nemomotz_update_buffer(pix->motz);
		nemomotz_detach_buffer(pix->motz);
	}

	nemocanvas_damage(canvas, 0, 0, pix->width, pix->height);
	nemocanvas_commit(canvas);
}

static int nemopix_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemopix *pix = (struct nemopix *)nemocanvas_get_userdata(canvas);
	struct actiontap *tap;

	if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		tap = nemoaction_tap_create(pix->action);
		nemoaction_tap_set_tx(tap, event->x);
		nemoaction_tap_set_ty(tap, event->y);
		nemoaction_tap_set_device(tap, event->device);
		nemoaction_tap_set_serial(tap, event->serial);
		nemoaction_tap_clear(tap, event->gx, event->gy);
		nemoaction_tap_dispatch_event(pix->action, tap, NEMOACTION_TAP_DOWN_EVENT);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		tap = nemoaction_get_tap_by_device(pix->action, event->device);
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, event->x);
			nemoaction_tap_set_ty(tap, event->y);
			nemoaction_tap_detach(tap);
			nemoaction_tap_dispatch_event(pix->action, tap, NEMOACTION_TAP_UP_EVENT);
			nemoaction_tap_destroy(tap);
		}
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		tap = nemoaction_get_tap_by_device(pix->action, event->device);
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, event->x);
			nemoaction_tap_set_ty(tap, event->y);
			nemoaction_tap_trace(tap, event->gx, event->gy);
			nemoaction_tap_dispatch_event(pix->action, tap, NEMOACTION_TAP_MOTION_EVENT);
		}
	}

	return 0;
}

static int nemopix_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	nemotool_exit(canvas->tool);

	return 1;
}

static int nemopix_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemopix *pix = (struct nemopix *)nemoaction_get_userdata(action);

	if ((event & NEMOACTION_TAP_DOWN_EVENT) || (event & NEMOACTION_TAP_UP_EVENT)) {
		struct actiontap *taps[8];
		int tap0, tap1;
		int ntaps;

		ntaps = nemoaction_get_taps_all(pix->action, taps, 8);
		if (ntaps == 1) {
			nemocanvas_move(pix->canvas,
					nemoaction_tap_get_serial(taps[0]));
		} else if (ntaps == 2) {
			nemocanvas_pick(pix->canvas,
					nemoaction_tap_get_serial(taps[0]),
					nemoaction_tap_get_serial(taps[1]),
					"rotate;scale;translate");
		} else if (ntaps >= 3) {
			nemoaction_get_distant_taps(pix->action, taps, ntaps, &tap0, &tap1);

			nemocanvas_pick(pix->canvas,
					nemoaction_tap_get_serial(taps[tap0]),
					nemoaction_tap_get_serial(taps[tap1]),
					"rotate;scale;translate");
		}
	}

	return 0;
}

static void nemopix_dispatch_alive_timer(struct nemotimer *timer, void *data)
{
	struct nemopix *pix = (struct nemopix *)data;

	nemotool_client_alive(pix->tool, pix->alive_timeout);

	nemotimer_set_timeout(pix->alive_timer, pix->alive_timeout / 2);
}

static void nemopix_dispatch_bus(void *data, const char *events)
{
	struct nemopix *pix = (struct nemopix *)data;
	struct nemojson *json;
	struct nemoitem *msg;
	struct itemone *one;
	char buffer[4096];
	int length;
	int i;

	length = nemobus_recv(pix->bus, buffer, sizeof(buffer));
	if (length <= 0)
		return;

	json = nemojson_create(buffer, length);
	nemojson_update(json);

	for (i = 0; i < nemojson_get_object_count(json); i++) {
		msg = nemoitem_create();
		nemoitem_load_json(msg, "/nemopix", nemojson_get_object(json, i));

		nemoitem_for_each(one, msg) {
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
		{ "opaque",				required_argument,		NULL,		'q' },
		{ "layer",				required_argument,		NULL,		'y' },
		{ "busid",				required_argument,		NULL,		'b' },
		{ "alive",				required_argument,		NULL,		'e' },
		{ 0 }
	};

	struct nemopix *pix;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct nemomotz *motz;
	struct motzone *one;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	char *busid = NULL;
	char *layer = NULL;
	int width = 500;
	int height = 500;
	int opaque = 1;
	int alive = 0;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:c:q:y:b:e:", options, NULL)) {
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

			case 'q':
				opaque = strcasecmp(optarg, "on") == 0;
				break;

			case 'y':
				layer = strdup(optarg);
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

	pix = (struct nemopix *)malloc(sizeof(struct nemopix));
	if (pix == NULL)
		return -1;
	memset(pix, 0, sizeof(struct nemopix));

	pix->width = width;
	pix->height = height;
	pix->opaque = opaque;
	pix->alive_timeout = alive;

	pix->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);

	pix->alive_timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemopix_dispatch_alive_timer);
	nemotimer_set_userdata(timer, pix);
	nemotimer_set_timeout(timer, pix->alive_timeout / 2);

	pix->canvas = canvas = nemocanvas_create(tool);
	nemocanvas_set_size(canvas, width, height);
	nemocanvas_set_nemosurface(canvas, "normal");
	nemocanvas_set_dispatch_resize(canvas, nemopix_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemopix_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemopix_dispatch_canvas_event);
	nemocanvas_set_dispatch_destroy(canvas, nemopix_dispatch_canvas_destroy);
	nemocanvas_set_fullscreen_type(canvas, "pick;pitch");
	nemocanvas_set_state(canvas, "close");
	nemocanvas_set_userdata(canvas, pix);

	if (opaque != 0)
		nemocanvas_opaque(canvas, 0, 0, width, height);

	if (layer != NULL)
		nemocanvas_set_layer(canvas, layer);

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	pix->action = action = nemoaction_create();
	nemoaction_set_tap_callback(action, nemopix_dispatch_tap_event);
	nemoaction_set_userdata(action, pix);

	if (busid != NULL) {
		pix->bus = nemobus_create();
		nemobus_connect(pix->bus, NULL);
		nemobus_advertise(pix->bus, "set", busid);

		nemotool_watch_source(tool,
				nemobus_get_socket(pix->bus),
				"reh",
				nemopix_dispatch_bus,
				pix);
	}

	pix->motz = motz = nemomotz_create();
	nemomotz_set_size(motz, width, height);

	one = nemomotz_object_create();
	nemomotz_object_set_shape(one, NEMOMOTZ_OBJECT_RECT_SHAPE);
	nemomotz_object_set_x(one, 50.0f);
	nemomotz_object_set_y(one, 50.0f);
	nemomotz_object_set_width(one, 150.0f);
	nemomotz_object_set_height(one, 150.0f);
	nemomotz_object_set_red(one, 0.0f);
	nemomotz_object_set_green(one, 255.0f);
	nemomotz_object_set_blue(one, 255.0f);
	nemomotz_object_set_alpha(one, 255.0f);
	nemomotz_object_set_stroke_width(one, 5.0f);
	nemomotz_object_set_sx(one, 1.5f);
	nemomotz_one_set_flags(one, NEMOMOTZ_OBJECT_STROKE_FLAG);
	nemomotz_attach_one(motz, one);

	nemocanvas_dispatch_frame(canvas);

	nemotool_run(tool);

	nemomotz_destroy(pix->motz);

	if (pix->bus != NULL)
		nemobus_destroy(pix->bus);

	nemoaction_destroy(action);

	nemotimer_destroy(timer);

	nemotool_destroy(tool);

	free(pix);

	return 0;
}
