#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>
#include <getopt.h>

#include <tesseract/capi.h>
#include <leptonica/allheaders.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoaction.h>
#include <nemotozz.h>
#include <nemomisc.h>

struct nemopen {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct nemotozz *tozz;

	int width, height;
	int opaque;

	TessBaseAPI *tess;
};

static void nemopen_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct nemopen *pen = (struct nemopen *)nemocanvas_get_userdata(canvas);

	if (width == 0 || height == 0)
		return;

	pen->width = width;
	pen->height = height;

	nemocanvas_set_size(canvas, width, height);

	if (pen->opaque != 0)
		nemocanvas_opaque(canvas, 0, 0, width, height);

	nemocanvas_dispatch_frame(canvas);
}

static void nemopen_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemopen *pen = (struct nemopen *)nemocanvas_get_userdata(canvas);
	pixman_image_t *framebuffer;

	nemocanvas_ready(canvas);

	framebuffer = nemocanvas_get_pixman_image(canvas);
	if (framebuffer != NULL) {
		nemotozz_attach_buffer(pen->tozz,
				NEMOTOZZ_CANVAS_RGBA_COLOR,
				NEMOTOZZ_CANVAS_PREMUL_ALPHA,
				pixman_image_get_data(framebuffer),
				pixman_image_get_width(framebuffer),
				pixman_image_get_height(framebuffer));
		nemotozz_clear(pen->tozz);
		nemotozz_detach_buffer(pen->tozz);

		nemocanvas_dispatch_feedback(canvas);

		nemocanvas_damage(canvas, 0, 0, pen->width, pen->height);
		nemocanvas_commit(canvas);
	}
}

static int nemopen_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemopen *pen = (struct nemopen *)nemocanvas_get_userdata(canvas);
	struct actiontap *tap;

	if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		tap = nemoaction_tap_create(pen->action);
		nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
		nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
		nemoaction_tap_set_device(tap, nemoevent_get_device(event));
		nemoaction_tap_set_serial(tap, nemoevent_get_serial(event));
		nemoaction_tap_clear(tap,
				nemoevent_get_global_x(event),
				nemoevent_get_global_y(event));
		nemoaction_tap_dispatch_event(pen->action, tap, NEMOACTION_TAP_DOWN_EVENT);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		tap = nemoaction_get_tap_by_device(pen->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_detach(tap);
			nemoaction_tap_dispatch_event(pen->action, tap, NEMOACTION_TAP_UP_EVENT);
			nemoaction_tap_destroy(tap);
		}
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		tap = nemoaction_get_tap_by_device(pen->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_trace(tap,
					nemoevent_get_global_x(event),
					nemoevent_get_global_y(event));
			nemoaction_tap_dispatch_event(pen->action, tap, NEMOACTION_TAP_MOTION_EVENT);
		}
	}

	return 0;
}

static int nemopen_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	nemotool_exit(nemocanvas_get_tool(canvas));

	return 1;
}

static int nemopen_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemopen *pen = (struct nemopen *)nemoaction_get_userdata(action);

	return 0;
}

int nemopen_initialize(struct nemopen *pen, const char *language)
{
	TessBaseAPI *tess;

	tess = TessBaseAPICreate();

	if (TessBaseAPIInit3(tess, NULL, language) != 0)
		goto err1;

	pen->tess = tess;

	return 0;

err1:
	TessBaseAPIEnd(tess);
	TessBaseAPIDelete(tess);

	return -1;
}

void nemopen_finalize(struct nemopen *pen)
{
	TessBaseAPIEnd(pen->tess);
	TessBaseAPIDelete(pen->tess);
}

int nemopen_recognize(struct nemopen *pen, const char *filepath)
{
	PIX *img;
	char *text;

	img = pixRead(filepath);
	if (img == NULL)
		return -1;

	TessBaseAPISetImage2(pen->tess, img);

	if (TessBaseAPIRecognize(pen->tess, NULL) == 0 && (text = TessBaseAPIGetUTF8Text(pen->tess)) != NULL) {
		NEMO_DEBUG("=> [%s]\n", text);

		TessDeleteText(text);
	}

	pixDestroy(&img);

	return 0;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,		NULL,		'w' },
		{ "height",				required_argument,		NULL,		'h' },
		{ "opaque",				required_argument,		NULL,		'q' },
		{ "layer",				required_argument,		NULL,		'y' },
		{ "fullscreen",		required_argument,		NULL,		'f' },
		{ 0 }
	};

	struct nemopen *pen;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemoaction *action;
	struct nemotozz *tozz;
	char *layer = NULL;
	char *fullscreenid = NULL;
	int width = 500;
	int height = 500;
	int opaque = 0;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:q:y:f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'q':
				opaque = strcasecmp(optarg, "on") == 0;
				break;

			case 'y':
				layer = strdup(optarg);
				break;

			case 'f':
				fullscreenid = strdup(optarg);
				break;

			default:
				break;
		}
	}

	pen = (struct nemopen *)malloc(sizeof(struct nemopen));
	if (pen == NULL)
		return -1;
	memset(pen, 0, sizeof(struct nemopen));

	pen->width = width;
	pen->height = height;
	pen->opaque = opaque;

	pen->tool = tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);

	pen->canvas = canvas = nemocanvas_create(tool);
	nemocanvas_set_size(canvas, width, height);
	nemocanvas_set_nemosurface(canvas, "normal");
	nemocanvas_set_dispatch_resize(canvas, nemopen_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(canvas, nemopen_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemopen_dispatch_canvas_event);
	nemocanvas_set_dispatch_destroy(canvas, nemopen_dispatch_canvas_destroy);
	nemocanvas_set_state(canvas, "close");
	nemocanvas_set_userdata(canvas, pen);

	if (opaque != 0)
		nemocanvas_opaque(canvas, 0, 0, width, height);
	if (layer != NULL)
		nemocanvas_set_layer(canvas, layer);
	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	pen->action = action = nemoaction_create();
	nemoaction_set_tap_callback(action, nemopen_dispatch_tap_event);
	nemoaction_set_userdata(action, pen);

	pen->tozz = tozz = nemotozz_create();

	nemopen_initialize(pen, "eng");

	nemocanvas_dispatch_frame(canvas);

	nemotool_run(tool);

	nemopen_finalize(pen);

	nemotozz_destroy(tozz);

	nemoaction_destroy(action);

	nemocanvas_destroy(canvas);

	nemotool_destroy(tool);

	free(pen);

	return 0;
}
