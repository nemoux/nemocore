#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <getopt.h>

#define	SK_RELEASE			1
#define	SK_CPU_LENDIAN	1
#define SK_R32_SHIFT		16
#define SK_G32_SHIFT		8
#define SK_B32_SHIFT		0
#define SK_A32_SHIFT		24

#include <SkTypes.h>
#include <SkCanvas.h>
#include <SkGraphics.h>
#include <SkImageInfo.h>
#include <SkImageEncoder.h>
#include <SkImageDecoder.h>
#include <SkBitmapDevice.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkString.h>
#include <SkMatrix.h>
#include <SkRegion.h>
#include <SkParsePath.h>
#include <SkTypeface.h>

#include <SkBlurMask.h>
#include <SkBlurMaskFilter.h>
#include <SkEmbossMaskFilter.h>

#include <SkGradientShader.h>

#include <SkGeometry.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoegl.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct touchcontext {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *back;
	struct talenode *node;

	int32_t width, height;

	double xtaps[128];
	double ytaps[128];
	char staps[128][32];
	int ntaps;

	int is_calibration;
	int corner;
};

static void nemotouch_render_touchpoints(struct touchcontext *touch, pixman_image_t *image)
{
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);
	int i;

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorBLACK);

	SkPaint stroke;
	stroke.setAntiAlias(true);
	stroke.setStyle(SkPaint::kStroke_Style);
	stroke.setColor(SK_ColorYELLOW);
	stroke.setStrokeWidth(3.0f);

	SkPaint paint;
	paint.setAntiAlias(true);
	paint.setStyle(SkPaint::kFill_Style);
	paint.setColor(SK_ColorWHITE);
	paint.setTypeface(SkTypeface::CreateFromFile("/usr/share/fonts/ttf/LiberationMono-Regular.ttf", 0));
	paint.setTextSize(16.0f);

	for (i = 0; i < touch->ntaps; i++) {
		canvas.drawCircle(touch->xtaps[i], touch->ytaps[i], 10.0f, stroke);

		canvas.drawText(
				touch->staps[i], strlen(touch->staps[i]),
				SkIntToScalar(touch->xtaps[i] + 10.0f), SkIntToScalar(touch->ytaps[i] - 10.0f),
				paint);
	}
}

static void nemotouch_render_corner(struct touchcontext *touch, pixman_image_t *image)
{
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);
	int i;

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorBLACK);

	SkPaint stroke;
	stroke.setAntiAlias(true);
	stroke.setStyle(SkPaint::kStroke_Style);
	stroke.setColor(SK_ColorYELLOW);
	stroke.setStrokeWidth(3.0f);

	if (touch->corner == 0) {
		canvas.drawCircle(width * 0.1f, height * 0.1f, 30.0f, stroke);
	} else if (touch->corner == 1) {
		canvas.drawCircle(width * 0.9f, height * 0.1f, 30.0f, stroke);
	} else if (touch->corner == 2) {
		canvas.drawCircle(width * 0.1f, height * 0.9f, 30.0f, stroke);
	} else if (touch->corner == 3) {
		canvas.drawCircle(width * 0.9f, height * 0.9f, 30.0f, stroke);
	}
}

static void nemotouch_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct touchcontext *touch = (struct touchcontext *)nemotale_get_userdata(tale);
	struct talenode *node = touch->node;

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	}

	if (touch->is_calibration == 0) {
		nemotouch_render_touchpoints(touch,
				nemotale_node_get_pixman(node));
	} else {
		nemotouch_render_corner(touch,
				nemotale_node_get_pixman(node));
	}

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);
}

static void nemotouch_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	struct touchcontext *touch = (struct touchcontext *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (touch->is_calibration == 0) {
		int i, ntaps = 0;

		nemotale_event_update_taps(tale, event, type);

		for (i = 0; i < event->tapcount; i++) {
			if (type == NEMOTALE_TOUCH_UP_EVENT && event->device == event->taps[i]->device)
				continue;

			touch->xtaps[ntaps] = event->taps[i]->x;
			touch->ytaps[ntaps] = event->taps[i]->y;

			snprintf(touch->staps[ntaps], 32, "%u", event->taps[i]->device);

			nemolog_message("TOUCH", "[%d TAP] %f %f\n", i, touch->xtaps[ntaps], touch->ytaps[ntaps]);

			ntaps++;
		}

		touch->ntaps = ntaps;

		nemocanvas_dispatch_frame(touch->canvas);
	} else {
		if (nemotale_is_long_press(tale, event, type)) {
			nemotale_tap_set_state(event->tap, NEMOTALE_TAP_USED_STATE);

			nemolog_message("TOUCH", "[%d CORNER] %f %f\n", touch->corner, event->x, event->y);

			if (++touch->corner >= 4) {
				nemotool_exit(touch->tool);
			}

			nemocanvas_dispatch_frame(touch->canvas);
		}
	}
}

static void nemotouch_dispatch_tale_timer(struct nemotimer *timer, void *data)
{
	struct touchcontext *touch = (struct touchcontext *)data;

	nemotimer_set_timeout(timer, 500);

	nemotale_push_timer_event(touch->tale, time_current_msecs());
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,			NULL,		'w' },
		{ "height",				required_argument,			NULL,		'h' },
		{ "uri",					required_argument,			NULL,		'u' },
		{ "calibration",	no_argument,						NULL,		'c' },
		{ 0 }
	};
	struct touchcontext *touch;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	int32_t width = 1920;
	int32_t height = 1080;
	char *uri = NULL;
	int is_calibration = 0;
	int opt;
	int i;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "w:h:u:c", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'u':
				uri = strdup(optarg);
				break;

			case 'c':
				is_calibration = 1;
				break;

			default:
				break;
		}
	}

	touch = (struct touchcontext *)malloc(sizeof(struct touchcontext));
	if (touch == NULL)
		return -1;
	memset(touch, 0, sizeof(struct touchcontext));

	touch->width = width;
	touch->height = height;

	touch->is_calibration = is_calibration;
	touch->corner = 0;

	touch->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	touch->egl = egl = nemotool_create_egl(tool);

	touch->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), nemotouch_dispatch_canvas_frame);

	touch->canvas = NTEGL_CANVAS(canvas);

	touch->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), nemotouch_dispatch_tale_event);
	nemotale_set_userdata(tale, touch);

	touch->back = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(touch->back, 0);
	nemotale_node_opaque(touch->back, 0, 0, width, height);
	nemotale_attach_node(tale, touch->back);

	touch->node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(touch->node, 1);
	nemotale_attach_node(tale, touch->node);

	timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemotouch_dispatch_tale_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, touch);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemotimer_destroy(timer);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(touch);

	return 0;
}
