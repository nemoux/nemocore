#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

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
#include <nemocanvas.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

static void plexback_render_one(pixman_image_t *image)
{
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);

	SkBitmap sbitmap;
	sbitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	sbitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice sdevice(sbitmap);
	SkCanvas scanvas(&sdevice);

	SkPaint paint;
	paint.setStyle(SkPaint::kStrokeAndFill_Style);
	paint.setStrokeWidth(3.0f);
	paint.setColor(
			SkColorSetARGB(
				random_get_double(0.0f, 255.0f),
				random_get_double(0.0f, 255.0f),
				random_get_double(0.0f, 255.0f),
				random_get_double(0.0f, 255.0f)));

	SkRect rect = SkRect::MakeXYWH(width * 0.1f, height * 0.1f, width * 0.8f, height * 0.8f);

	scanvas.save();
	scanvas.drawRect(rect, paint);
}

static void plexback_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else {
		nemocanvas_feedback(canvas);
	}

	nemocanvas_buffer(canvas);

	plexback_render_one(nemocanvas_get_pixman_image(canvas));

	nemocanvas_damage(canvas, 0, 0, 0, 0);
	nemocanvas_commit(canvas);
}

static int plexback_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	return 0;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "background",	no_argument,						NULL,		'b' },
		{ 0 }
	};
	struct nemotool *tool;
	struct nemocanvas *canvas;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;

	while (opt = getopt_long(argc, argv, "w:h:b", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	canvas = nemocanvas_create(tool);
	nemocanvas_set_nemosurface(canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_layer(canvas, NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_frame(canvas, plexback_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, plexback_dispatch_canvas_event);

	nemocanvas_set_size(canvas, width, height);

	nemocanvas_dispatch_frame(canvas);

	nemotool_run(tool);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	return 0;
}
