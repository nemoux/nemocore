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

#include <moteback.h>
#include <nemotool.h>
#include <nemoegl.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <nemomisc.h>

static void moteback_update_one(struct moteback *mote, double secs)
{
	nemomote_blast_update(&mote->mote, &mote->blastemitter, secs);
	nemomote_positionbuilder_update(&mote->mote, secs, &mote->zone);
	nemomote_commit(&mote->mote);

	nemomote_gravitywallactor_update(&mote->mote, secs, 960.0f, 270.0f, 0.0f, 10000.0f, 10000.0f);
	nemomote_gravitywallactor_update(&mote->mote, secs, 960.0f, 810.0f, 0.0f, 10000.0f, 10000.0f);
	nemomote_gravitywallactor_update(&mote->mote, secs, 2880.0f, 280.0f, 0.0f, 10000.0f, 10000.0f);
	nemomote_gravitywallactor_update(&mote->mote, secs, 2800.0f, 810.0f, 0.0f, 10000.0f, 10000.0f);
	nemomote_moveactor_update(&mote->mote, secs);
	nemomote_cleanup(&mote->mote);
}

static void moteback_render_one(struct moteback *mote, pixman_image_t *image)
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

	canvas.clear(SK_ColorTRANSPARENT);

	SkMaskFilter *filter = SkBlurMaskFilter::Create(
			kSolid_SkBlurStyle,
			SkBlurMask::ConvertRadiusToSigma(5.0f),
			SkBlurMaskFilter::kHighQuality_BlurFlag);

	SkPaint stroke;
	stroke.setAntiAlias(true);
	stroke.setStyle(SkPaint::kStroke_Style);
	stroke.setStrokeWidth(1.0f);
	stroke.setColor(SkColorSetARGB(63.0f, 0.0f, 255.0f, 255.0f));
	stroke.setMaskFilter(filter);

	SkPaint fill;
	fill.setAntiAlias(true);
	fill.setStyle(SkPaint::kFill_Style);
	fill.setColor(SkColorSetARGB(128.0f, 0.0f, 255.0f, 255.0f));
	fill.setMaskFilter(filter);

	for (i = 0; i < nemomote_get_count(&mote->mote); i++) {
		canvas.drawCircle(
				NEMOMOTE_POSITION_X(&mote->mote, i),
				NEMOMOTE_POSITION_Y(&mote->mote, i),
				5.0f,
				fill);
	}
}

static void moteback_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct moteback *mote = (struct moteback *)nemotale_get_userdata(tale);
	struct talenode *node = mote->node;

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else {
		nemocanvas_feedback(canvas);

		if (mote->secs == 0.0f)
			moteback_update_one(mote, 0.0f);
		else
			moteback_update_one(mote, (secs + nsecs / 1000000000) - mote->secs);

		mote->secs = secs + nsecs / 1000000000;
	}

	moteback_render_one(mote, nemotale_node_get_pixman(node));

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);
}

static void moteback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "background",	no_argument,						NULL,		'b' },
		{ 0 }
	};
	struct moteback *mote;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;
	int i;

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

	mote = (struct moteback *)malloc(sizeof(struct moteback));
	if (mote == NULL)
		return -1;
	memset(mote, 0, sizeof(struct moteback));

	mote->width = width;
	mote->height = height;

	nemomote_init(&mote->mote);
	nemomote_set_max_particles(&mote->mote, 3000);
	nemomote_blast_set_property(&mote->blastemitter, 300);
	nemomote_blast_ready(&mote->mote, &mote->blastemitter);
	nemozone_set_cube(&mote->zone, 0, 3840, 0, 1920, 0, 0);

	mote->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	mote->egl = egl = nemotool_create_egl(tool);

	mote->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_layer(NTEGL_CANVAS(canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), moteback_dispatch_canvas_frame);

	mote->canvas = NTEGL_CANVAS(canvas);

	mote->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), moteback_dispatch_tale_event);
	nemotale_set_userdata(tale, mote);

	mote->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);
	nemotale_attach_node(tale, node);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(mote);

	return 0;
}
