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
#include <nemoegl.h>
#include <nemomosi.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <nemomosi.h>
#include <nemomisc.h>

struct mosiback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	pixman_image_t *imgs[8];
	int nimgs, iimgs;

	int type;
	double r;
	struct nemomosi *mosi;
};

static int mosiback_render(struct mosiback *mosi, pixman_image_t *image)
{
	struct mosione *one;
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);
	int i, j;

	SkBitmap bitmap;
	bitmap.setInfo(
			SkImageInfo::Make(width, height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			pixman_image_get_data(image));

	SkBitmapDevice device(bitmap);
	SkCanvas canvas(&device);

	canvas.clear(SK_ColorTRANSPARENT);

	SkPaint fill;
	fill.setAntiAlias(true);
	fill.setStyle(SkPaint::kFill_Style);

	for (i = 0; i < nemomosi_get_height(mosi->mosi); i++) {
		for (j = 0; j < nemomosi_get_width(mosi->mosi); j++) {
			one = nemomosi_get_one(mosi->mosi, j, i);

			fill.setColor(
					SkColorSetARGB(
						one->c[3],
						one->c[2],
						one->c[1],
						one->c[0]));

			canvas.drawCircle(
					mosi->r + j * mosi->r * 2.0f,
					mosi->r + i * mosi->r * 2.0f,
					mosi->r,
					fill);
		}
	}

	return 0;
}

static void mosiback_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct mosiback *mosi = (struct mosiback *)nemotale_get_userdata(tale);
	struct talenode *node = mosi->node;
	uint32_t msecs = time_current_msecs();
	int done;

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else {
		nemocanvas_feedback(canvas);
	}

	done = nemomosi_update(mosi->mosi, msecs);
	if (done != 0) {
		mosi->iimgs = (mosi->iimgs + 1) % mosi->nimgs;

		nemomosi_tween_image(mosi->mosi, (uint8_t *)pixman_image_get_data(mosi->imgs[mosi->iimgs]));

		if (mosi->type == 0) {
			nemomosi_wave_dispatch(mosi->mosi, msecs,
					random_get_int(0, nemomosi_get_width(mosi->mosi)),
					random_get_int(0, nemomosi_get_height(mosi->mosi)),
					(nemomosi_get_width(mosi->mosi) + nemomosi_get_height(mosi->mosi)),
					0, 5000,
					500, 1000);
			nemomosi_wave_dispatch(mosi->mosi, msecs,
					random_get_int(0, nemomosi_get_width(mosi->mosi)),
					random_get_int(0, nemomosi_get_height(mosi->mosi)),
					(nemomosi_get_width(mosi->mosi) + nemomosi_get_height(mosi->mosi)),
					0, 5000,
					500, 1000);

			mosi->type = 1;
		} else if (mosi->type == 1) {
			nemomosi_oneshot_dispatch(mosi->mosi, msecs, 0, 5000);

			mosi->type = 2;
		} else if (mosi->type == 2) {
			nemomosi_rain_dispatch(mosi->mosi, msecs, 0, 5000, 10000, 0.1f, 0.3f);

			mosi->type = 3;
		} else if (mosi->type == 3) {
			nemomosi_flip_dispatch(mosi->mosi, msecs, 0, 10, 500, 1000);

			mosi->type = 4;
		} else if (mosi->type == 4) {
			mosi->type = 5;
		} else {
			nemomosi_random_dispatch(mosi->mosi, msecs, 0, 5000, 500, 1500);

			mosi->type = 0;
		}
	}

	if (mosi->type == 5) {
		int x, y;
		int i;

		for (i = 0; i < 8; i++) {
			if (nemomosi_get_empty(mosi->mosi, &x, &y) > 0) {
				nemomosi_cross_dispatch(mosi->mosi, msecs,
						x, y,
						0,
						500, 1500,
						0.1f, 0.3f);
			}
		}
	}

	mosiback_render(mosi, nemotale_node_get_pixman(node));

	nemotale_node_damage_all(node);

	nemotale_composite_egl(tale, NULL);
}

static void mosiback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "srcfile",		required_argument,			NULL,		's' },
		{ "dstfile",		required_argument,			NULL,		'd' },
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "background",	no_argument,						NULL,		'b' },
		{ 0 }
	};
	struct mosiback *mosi;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	char *srcpath = NULL;
	char *dstpath = NULL;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "s:d:w:h:b", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				srcpath = strdup(optarg);
				break;

			case 'd':
				dstpath = strdup(optarg);
				break;

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

	if (srcpath == NULL || dstpath == NULL)
		return 0;

	mosi = (struct mosiback *)malloc(sizeof(struct mosiback));
	if (mosi == NULL)
		return -1;
	memset(mosi, 0, sizeof(struct mosiback));

	mosi->width = width;
	mosi->height = height;

	mosi->mosi = nemomosi_create(
			128 * ((double)width / (double)height),
			128);
	mosi->r = (double)height / 128.0f / 2.0f;

	mosi->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	mosi->egl = egl = nemotool_create_egl(tool);

	mosi->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_layer(NTEGL_CANVAS(canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), mosiback_dispatch_canvas_frame);

	mosi->canvas = NTEGL_CANVAS(canvas);

	mosi->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), mosiback_dispatch_tale_event);
	nemotale_set_userdata(tale, mosi);

	mosi->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);
	nemotale_attach_node(tale, node);

	mosi->imgs[0] = pixman_load_image(srcpath, nemomosi_get_width(mosi->mosi), nemomosi_get_height(mosi->mosi));
	mosi->imgs[1] = pixman_load_image(dstpath, nemomosi_get_width(mosi->mosi), nemomosi_get_height(mosi->mosi));
	mosi->nimgs = 2;
	mosi->iimgs = 0;

	nemomosi_clear_one(mosi->mosi, 0, 0, 0, 0);

	nemomosi_tween_image(mosi->mosi, (uint8_t *)pixman_image_get_data(mosi->imgs[0]));
	nemomosi_random_dispatch(mosi->mosi, time_current_msecs(), 0, 5000, 500, 1500);

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	nemomosi_destroy(mosi->mosi);

	free(mosi);

	return 0;
}
