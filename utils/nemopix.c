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
#include <motzgroup.h>
#include <motzobject.h>
#include <motzpath.h>
#include <motzclip.h>
#include <motzburst.h>
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

	nemomotz_update(pix->motz, time_current_msecs());

	if (nemomotz_has_flags(pix->motz, NEMOMOTZ_REDRAW_FLAG) != 0) {
		nemomotz_put_flags(pix->motz, NEMOMOTZ_REDRAW_FLAG);

		nemocanvas_ready(canvas);

		framebuffer = nemocanvas_get_pixman_image(canvas);
		if (framebuffer != NULL) {
			nemomotz_attach_buffer(pix->motz,
					pixman_image_get_data(framebuffer),
					pixman_image_get_width(framebuffer),
					pixman_image_get_height(framebuffer));
			nemomotz_update_buffer(pix->motz);
			nemomotz_detach_buffer(pix->motz);
		}

		nemocanvas_dispatch_feedback(canvas);

		nemocanvas_damage(canvas, 0, 0, pix->width, pix->height);
		nemocanvas_commit(canvas);
	} else {
		nemocanvas_terminate_feedback(canvas);
	}
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
		if (ntaps == 2) {
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

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		nemomotz_dispatch_down_event(pix->motz,
				nemoaction_tap_get_device(tap),
				nemoaction_tap_get_tx(tap),
				nemoaction_tap_get_ty(tap));
		nemocanvas_dispatch_frame(pix->canvas);
	} else if (event & NEMOACTION_TAP_MOTION_EVENT) {
		nemomotz_dispatch_motion_event(pix->motz,
				nemoaction_tap_get_device(tap),
				nemoaction_tap_get_tx(tap),
				nemoaction_tap_get_ty(tap));
		nemocanvas_dispatch_frame(pix->canvas);
	} else if (event & NEMOACTION_TAP_UP_EVENT) {
		nemomotz_dispatch_up_event(pix->motz,
				nemoaction_tap_get_device(tap),
				nemoaction_tap_get_tx(tap),
				nemoaction_tap_get_ty(tap));
		nemocanvas_dispatch_frame(pix->canvas);
	}

	return 0;
}

static void nemopix_dispatch_motz_down(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
	struct motzone *one;

	one = nemomotz_burst_create();
	nemomotz_burst_set_size(one, 25.0f);
	nemomotz_burst_set_red(one, 0.0f);
	nemomotz_burst_set_green(one, 255.0f);
	nemomotz_burst_set_blue(one, 255.0f);
	nemomotz_burst_set_alpha(one, 255.0f);
	nemomotz_burst_set_stroke_width(one, 6.5f);
	nemomotz_attach_one(motz, one);

	nemomotz_tap_set_one(tap, one);

	nemomotz_one_down(motz, tap, one, x, y);
}

static void nemopix_dispatch_motz_motion(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
}

static void nemopix_dispatch_motz_up(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
}

static void nemopix_dispatch_motz_one_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemopix_dispatch_motz_one_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemopix_dispatch_motz_one_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
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
	struct motzone *group;
	struct motzone *clip;
	struct motzone *one;
	struct motztrans *trans;
	struct toyzstyle *style;
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
	nemomotz_set_down_callback(motz, nemopix_dispatch_motz_down);
	nemomotz_set_motion_callback(motz, nemopix_dispatch_motz_motion);
	nemomotz_set_up_callback(motz, nemopix_dispatch_motz_up);
	nemomotz_set_size(motz, width, height);
	nemomotz_set_userdata(motz, pix);

	group = nemomotz_group_create();
	nemomotz_group_set_tx(group, 0.0f);
	nemomotz_group_set_ty(group, 0.0f);
	nemomotz_attach_one(motz, group);

	one = nemomotz_object_create();
	nemomotz_one_set_down_callback(one, nemopix_dispatch_motz_one_down);
	nemomotz_one_set_motion_callback(one, nemopix_dispatch_motz_one_motion);
	nemomotz_one_set_up_callback(one, nemopix_dispatch_motz_one_up);
	nemomotz_object_set_shape(one, NEMOMOTZ_OBJECT_RECT_SHAPE);
	nemomotz_object_set_x(one, 0.0f);
	nemomotz_object_set_y(one, 0.0f);
	nemomotz_object_set_width(one, 150.0f);
	nemomotz_object_set_height(one, 150.0f);
	nemomotz_object_set_red(one, 0.0f);
	nemomotz_object_set_green(one, 255.0f);
	nemomotz_object_set_blue(one, 255.0f);
	nemomotz_object_set_alpha(one, 255.0f);
	nemomotz_object_set_stroke_width(one, 5.0f);
	nemomotz_object_set_tx(one, 50.0f);
	nemomotz_object_set_ty(one, 50.0f);
	nemomotz_object_set_sx(one, 1.5f);
	nemomotz_one_set_flags(one, NEMOMOTZ_OBJECT_STROKE_FLAG);
	nemomotz_one_attach_one(group, one);

	trans = nemomotz_transition_create(8, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 300);
	nemomotz_transition_object_set_red(trans, 0, one);
	nemomotz_transition_set_target(trans, 0, 1.0f, 255.0f);
	nemomotz_transition_object_set_green(trans, 1, one);
	nemomotz_transition_set_target(trans, 1, 1.0f, 0.0f);
	nemomotz_attach_transition(motz, trans);

	clip = nemomotz_clip_create();
	nemomotz_clip_set_x(clip, 0.0f);
	nemomotz_clip_set_y(clip, 0.0f);
	nemomotz_clip_set_width(clip, width);
	nemomotz_clip_set_height(clip, 96.0f);
	nemomotz_one_attach_one(group, clip);

	one = nemomotz_object_create();
	nemomotz_object_set_shape(one, NEMOMOTZ_OBJECT_TEXT_SHAPE);
	nemomotz_object_set_x(one, 0.0f);
	nemomotz_object_set_y(one, 0.0f);
	nemomotz_object_set_red(one, 0.0f);
	nemomotz_object_set_green(one, 255.0f);
	nemomotz_object_set_blue(one, 255.0f);
	nemomotz_object_set_alpha(one, 255.0f);
	nemomotz_object_set_stroke_width(one, 1.0f);
	nemomotz_one_set_flags(one, NEMOMOTZ_OBJECT_STROKE_FLAG);
	nemomotz_object_set_font_path(one, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
	nemomotz_object_set_font_size(one, 32.0f);
	nemomotz_object_set_text(one, "NEMO-pix");
	nemomotz_one_attach_one(clip, one);

	style = nemotoyz_style_create();
	nemotoyz_style_load_font(style,
			nemomotz_object_get_font_path(one),
			nemomotz_object_get_font_index(one));
	nemotoyz_style_set_font_size(style,
			nemomotz_object_get_font_size(one));

	one = nemomotz_object_create();
	nemomotz_object_set_shape(one, NEMOMOTZ_OBJECT_RECT_SHAPE);
	nemomotz_object_set_x(one, 0.0f);
	nemomotz_object_set_y(one, 0.0f);
	nemomotz_object_set_width(one,
			nemotoyz_style_get_text_width(style, "NEMO-pix", 8));
	nemomotz_object_set_height(one,
			nemotoyz_style_get_text_height(style));
	nemomotz_object_set_red(one, 0.0f);
	nemomotz_object_set_green(one, 255.0f);
	nemomotz_object_set_blue(one, 255.0f);
	nemomotz_object_set_alpha(one, 255.0f);
	nemomotz_object_set_stroke_width(one, 1.0f);
	nemomotz_one_set_flags(one, NEMOMOTZ_OBJECT_STROKE_FLAG);
	nemomotz_one_attach_one(clip, one);

	nemotoyz_style_destroy(style);

	trans = nemomotz_transition_create(8, NEMOEASE_CUBIC_INOUT_TYPE, 800, 150);
	nemomotz_transition_object_set_ty(trans, 0, one);
	nemomotz_transition_set_target(trans, 0, 1.0f, 96.0f);
	nemomotz_attach_transition(motz, trans);

	one = nemomotz_path_create();
	nemomotz_path_clear(one);
	nemomotz_path_moveto(one, 0.0f, 0.0f);
	nemomotz_path_lineto(one, width, 0.0f);
	nemomotz_path_lineto(one, width, height);
	nemomotz_path_lineto(one, 0.0f, 0.0f);
	nemomotz_path_lineto(one, 0.0f, height);
	nemomotz_path_lineto(one, width, height);
	nemomotz_path_close(one);
	nemomotz_path_set_red(one, 0.0f);
	nemomotz_path_set_green(one, 255.0f);
	nemomotz_path_set_blue(one, 255.0f);
	nemomotz_path_set_alpha(one, 255.0f);
	nemomotz_path_set_stroke_width(one, 3.0f);
	nemomotz_path_set_to(one, 0.0f);
	nemomotz_one_set_flags(one, NEMOMOTZ_PATH_STROKE_FLAG);
	nemomotz_one_set_contain_callback(one, NULL);
	nemomotz_attach_one(motz, one);

	trans = nemomotz_transition_create(8, NEMOEASE_CUBIC_INOUT_TYPE, 1200, 150);
	nemomotz_transition_path_set_from(trans, 0, one);
	nemomotz_transition_set_target(trans, 0, 0.5f, 0.25f);
	nemomotz_transition_set_target(trans, 0, 1.0f, 1.0f);
	nemomotz_transition_path_set_to(trans, 1, one);
	nemomotz_transition_set_target(trans, 1, 0.5f, 0.75f);
	nemomotz_transition_set_target(trans, 1, 1.0f, 1.0f);
	nemomotz_attach_transition(motz, trans);

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
