#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showcanvas.h>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>
#include <skiaconfig.hpp>

struct showone *nemoshow_canvas_create(void)
{
	struct showcanvas *canvas;
	struct showone *one;

	canvas = (struct showcanvas *)malloc(sizeof(struct showcanvas));
	if (canvas == NULL)
		return NULL;
	memset(canvas, 0, sizeof(struct showcanvas));

	one = &canvas->base;
	one->type = NEMOSHOW_CANVAS_TYPE;
	one->destroy = nemoshow_canvas_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "type", canvas->type, NEMOSHOW_CANVAS_TYPE_MAX);
	nemoobject_set_reserved(&one->object, "src", canvas->src, NEMOSHOW_CANVAS_SRC_MAX);
	nemoobject_set_reserved(&one->object, "event", &canvas->event, sizeof(int32_t));
	nemoobject_set_reserved(&one->object, "width", &canvas->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &canvas->height, sizeof(double));

	canvas->shapes = (struct showone **)malloc(sizeof(struct showone *) * 8);
	canvas->nshapes = 0;
	canvas->sshapes = 8;

	return one;
}

void nemoshow_canvas_destroy(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_one_finish(one);

	free(canvas->shapes);
	free(canvas);
}

static int nemoshow_canvas_compare(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

int nemoshow_canvas_arrange(struct nemoshow *show, struct showone *one)
{
	static struct canvasmap {
		char name[32];

		int type;
	} maps[] = {
		{ "img",				NEMOSHOW_CANVAS_IMAGE_TYPE },
		{ "opengl",			NEMOSHOW_CANVAS_OPENGL_TYPE },
		{ "pixman",			NEMOSHOW_CANVAS_PIXMAN_TYPE },
		{ "ref",				NEMOSHOW_CANVAS_REF_TYPE },
		{ "scene",			NEMOSHOW_CANVAS_SCENE_TYPE },
		{ "svg",				NEMOSHOW_CANVAS_SVG_TYPE },
		{ "use",				NEMOSHOW_CANVAS_USE_TYPE },
		{ "vec",				NEMOSHOW_CANVAS_VECTOR_TYPE },
	}, *map;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	map = static_cast<struct canvasmap *>(bsearch(canvas->type, static_cast<void *>(maps), sizeof(maps) / sizeof(maps[0]), sizeof(maps[0]), nemoshow_canvas_compare));
	if (map == NULL)
		one->sub = NEMOSHOW_CANVAS_NONE_TYPE;
	else
		one->sub = map->type;

	canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);

	if (canvas->event == 0) {
		nemotale_node_set_pick_type(canvas->node, NEMOTALE_PICK_NO_TYPE);
	} else {
		nemotale_node_set_id(canvas->node, canvas->event);
	}

	return 0;
}

static int nemoshow_canvas_update_vector(struct nemoshow *show, struct showcanvas *canvas)
{
	SkAutoGraphics autograph;
	SkBitmap bitmap;
	struct showone *one;
	int i;

	bitmap.setInfo(
			SkImageInfo::Make(canvas->width, canvas->height, kN32_SkColorType, kPremul_SkAlphaType));
	bitmap.setPixels(
			nemotale_node_get_buffer(canvas->node));

	SkBitmapDevice sdevice(bitmap);
	SkCanvas scanvas(&sdevice);

	SkPaint spaint;
	spaint.setStyle(SkPaint::kStrokeAndFill_Style);
	spaint.setStrokeWidth(3.0f);
	spaint.setColor(SK_ColorYELLOW);

	for (i = 0; i < canvas->nshapes; i++) {
		one = canvas->shapes[i];

		if (one->sub == NEMOSHOW_SHAPE_RECT_TYPE) {
			struct showrect *rect = NEMOSHOW_RECT(one);
			SkRect srect = SkRect::MakeXYWH(rect->x, rect->y, rect->width, rect->height);

			scanvas.drawRect(srect, spaint);
		}
	}

	return 0;
}

int nemoshow_canvas_update(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
		nemoshow_canvas_update_vector(show, canvas);
	} else if (one->sub == NEMOSHOW_CANVAS_SVG_TYPE) {
	} else {
		nemotale_node_fill_pixman(canvas->node, 0.0f, 1.0f, 1.0f, 1.0f);
	}

	return 0;
}
