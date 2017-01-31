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

	struct nemotimer *alive_timer;
	int alive_timeout;

	int width, height;
	int opaque;

	struct cookegl *egl;
};

static void nemopix_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemopix *pix = (struct nemopix *)nemocanvas_get_userdata(canvas);

	if (width == 0 || height == 0)
		return;

	pix->width = width;
	pix->height = height;

	nemocanvas_egl_resize(pix->canvas, width, height);

	if (pix->opaque != 0)
		nemocanvas_opaque(pix->canvas, 0, 0, width, height);

	nemocook_egl_resize(pix->egl, width, height);

	nemocanvas_dispatch_frame(canvas);
}

static void nemopix_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemopix *pix = (struct nemopix *)nemocanvas_get_userdata(canvas);

	nemocook_egl_make_current(pix->egl);

	nemocook_clear_color_buffer(0.0f, 0.0f, 0.0f, 0.0f);

	nemocook_egl_swap_buffers(pix->egl);
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
	struct cookegl *egl;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	char *busid = NULL;
	char *layer = NULL;
	int width = 1920;
	int height = 1080;
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

	if (contentpath == NULL && busid == NULL)
		return 0;

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
	nemotool_connect_egl(tool);

	pix->alive_timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemopix_dispatch_alive_timer);
	nemotimer_set_userdata(timer, pix);
	nemotimer_set_timeout(timer, pix->alive_timeout / 2);

	pix->canvas = canvas = nemocanvas_egl_create(tool, width, height);
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

	pix->egl = egl = nemocook_egl_create(
			NTEGL_DISPLAY(tool),
			NTEGL_CONTEXT(tool),
			NTEGL_CONFIG(tool),
			NTEGL_WINDOW(canvas));
	nemocook_egl_resize(egl, width, height);

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

	nemotool_run(tool);

	if (pix->bus != NULL)
		nemobus_destroy(pix->bus);

	nemocook_egl_destroy(egl);

	nemoaction_destroy(action);

	nemocanvas_egl_destroy(canvas);

	nemotimer_destroy(timer);

	nemotool_destroy(tool);

	free(pix);

	return 0;
}
