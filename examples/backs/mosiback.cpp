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

#include <mosiback.h>
#include <nemotool.h>
#include <nemoegl.h>
#include <pixmanhelper.h>
#include <talehelper.h>
#include <nemomisc.h>

static pixman_image_t *mosiback_prepare_image(const char *filepath, int32_t width, int32_t height)
{
	pixman_image_t *src;
	pixman_image_t *dst;
	pixman_transform_t transform;

	src = pixman_load_png_file(filepath);
	if (src == NULL)
		src = pixman_load_jpeg_file(filepath);
	if (src == NULL)
		exit(1);

	dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
			width, height,
			NULL,
			width * 4);

	pixman_transform_init_identity(&transform);
	pixman_transform_scale(&transform, NULL,
			pixman_double_to_fixed(
				(double)pixman_image_get_width(src) / (double)pixman_image_get_width(dst)),
			pixman_double_to_fixed(
				(double)pixman_image_get_height(src) / (double)pixman_image_get_height(dst)));

	pixman_image_set_transform(src, &transform);

	pixman_image_composite32(PIXMAN_OP_SRC,
			src,
			NULL,
			dst,
			0, 0, 0, 0, 0, 0,
			width, height);

	pixman_image_unref(src);

	return dst;
}

static void mosiback_prepare_one(struct mosiback *mosi, pixman_image_t *image, uint32_t msecs)
{
	struct mosione *one;
	uint8_t *src = (uint8_t *)pixman_image_get_data(image);
	int i, j;

	for (i = 0; i < mosi->row; i++) {
		for (j = 0; j < mosi->col; j++, src += 4) {
			one = &mosi->ones[i * mosi->col + j];

			one->c[3] = src[3];
			one->c[2] = src[2];
			one->c[1] = src[1];
			one->c[0] = src[0];

			one->c0[3] = one->c1[3] = one->c[3];
			one->c0[2] = one->c1[2] = one->c[2];
			one->c0[1] = one->c1[1] = one->c[1];
			one->c0[0] = one->c1[0] = one->c[0];

			one->x = mosi->radius + j * mosi->radius * 2.0f;
			one->y = mosi->radius + i * mosi->radius * 2.0f;
			one->r = 0.0f;

			one->stime = msecs + random_get_int(0, 10000);
			one->etime = one->stime + random_get_int(500, 1500);
		}
	}
}

static void mosiback_prepare_two(struct mosiback *mosi, pixman_image_t *image, uint32_t msecs)
{
	struct mosione *one;
	uint8_t *dst = (uint8_t *)pixman_image_get_data(image);
	int i, j;

	for (i = 0; i < mosi->row; i++) {
		for (j = 0; j < mosi->col; j++, dst += 4) {
			one = &mosi->ones[i * mosi->col + j];

			one->c0[3] = one->c[3];
			one->c0[2] = one->c[2];
			one->c0[1] = one->c[1];
			one->c0[0] = one->c[0];

			one->c1[3] = dst[3];
			one->c1[2] = dst[2];
			one->c1[1] = dst[1];
			one->c1[0] = dst[0];

			one->x = mosi->radius + j * mosi->radius * 2.0f;
			one->y = mosi->radius + i * mosi->radius * 2.0f;

			one->stime = msecs + random_get_int(0, 10000);
			one->etime = one->stime + random_get_int(500, 1500);
		}
	}
}

static int mosiback_render_one(struct mosiback *mosi, pixman_image_t *image, uint32_t msecs)
{
	struct mosione *one;
	int32_t width = pixman_image_get_width(image);
	int32_t height = pixman_image_get_height(image);
	uint8_t *src = (uint8_t *)pixman_image_get_data(image);
	int done = 1;
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

	for (i = 0; i < mosi->row; i++) {
		for (j = 0; j < mosi->col; j++, src += 4) {
			one = &mosi->ones[i * mosi->col + j];

			if (msecs < one->stime) {
				done = 0;
			} else if (msecs >= one->stime && msecs < one->etime) {
				double t = ((double)(msecs - one->stime) / (double)(one->etime - one->stime));

				one->r = mosi->radius;

				one->c[3] = (one->c1[3] - one->c0[3]) * t + one->c0[3];
				one->c[2] = (one->c1[2] - one->c0[2]) * t + one->c0[2];
				one->c[1] = (one->c1[1] - one->c0[1]) * t + one->c0[1];
				one->c[0] = (one->c1[0] - one->c0[0]) * t + one->c0[0];

				done = 0;
			} else if (msecs >= one->etime) {
				one->c[3] = one->c1[3];
				one->c[2] = one->c1[2];
				one->c[1] = one->c1[1];
				one->c[0] = one->c1[0];
			}

			fill.setColor(
					SkColorSetARGB(
						one->c[3],
						one->c[2],
						one->c[1],
						one->c[0]));

			canvas.drawCircle(one->x, one->y, one->r, fill);
		}
	}

	return done;
}

static void mosiback_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct mosiback *mosi = (struct mosiback *)nemotale_get_userdata(tale);
	struct talenode *node = mosi->node;
	int done;

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else {
		nemocanvas_feedback(canvas);
	}

	done = mosiback_render_one(mosi, nemotale_node_get_pixman(node), time_current_msecs());
	if (done != 0) {
		mosi->iimgs = (mosi->iimgs + 1) % mosi->nimgs;

		mosiback_prepare_two(mosi, mosi->imgs[mosi->iimgs], time_current_msecs());
	}

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

	mosi->row = 128;
	mosi->col = 128 * ((double)width / (double)height);
	mosi->radius = (double)height / 128.0f / 2.0f;

	mosi->ones = (struct mosione *)malloc(sizeof(struct mosione) * mosi->row * mosi->col);
	if (mosi->ones == NULL)
		return -1;
	memset(mosi->ones, 0, sizeof(struct mosione) * mosi->row * mosi->col);

	mosi->nones = mosi->row * mosi->col;

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

	mosi->imgs[0] = mosiback_prepare_image(srcpath, mosi->col, mosi->row);
	mosi->imgs[1] = mosiback_prepare_image(dstpath, mosi->col, mosi->row);
	mosi->nimgs = 2;
	mosi->iimgs = 0;

	mosiback_prepare_one(mosi, mosi->imgs[0], time_current_msecs());

	nemocanvas_dispatch_frame(NTEGL_CANVAS(canvas));

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(mosi->ones);
	free(mosi);

	return 0;
}
