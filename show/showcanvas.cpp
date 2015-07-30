#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showcanvas.h>
#include <showcanvas.hpp>
#include <showitem.h>
#include <showitem.hpp>
#include <showmatrix.h>
#include <showmatrix.hpp>
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

	canvas->cc = new showcanvas_t;

	one = &canvas->base;
	one->type = NEMOSHOW_CANVAS_TYPE;
	one->destroy = nemoshow_canvas_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "type", canvas->type, NEMOSHOW_CANVAS_TYPE_MAX);
	nemoobject_set_reserved(&one->object, "src", canvas->src, NEMOSHOW_CANVAS_SRC_MAX);
	nemoobject_set_reserved(&one->object, "event", &canvas->event, sizeof(int32_t));
	nemoobject_set_reserved(&one->object, "width", &canvas->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &canvas->height, sizeof(double));

	canvas->items = (struct showone **)malloc(sizeof(struct showone *) * 8);
	canvas->nitems = 0;
	canvas->sitems = 8;

	return one;
}

void nemoshow_canvas_destroy(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_one_finish(one);

	delete static_cast<showcanvas_t *>(canvas->cc);

	free(canvas->items);
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

	NEMOSHOW_CANVAS_CC(canvas, bitmap) = new SkBitmap;

	NEMOSHOW_CANVAS_CC(canvas, bitmap)->setInfo(
			SkImageInfo::Make(canvas->width, canvas->height, kN32_SkColorType, kPremul_SkAlphaType));
	NEMOSHOW_CANVAS_CC(canvas, bitmap)->setPixels(
			nemotale_node_get_buffer(canvas->node));

	NEMOSHOW_CANVAS_CC(canvas, device) = new SkBitmapDevice(*NEMOSHOW_CANVAS_CC(canvas, bitmap));
	NEMOSHOW_CANVAS_CC(canvas, canvas) = new SkCanvas(NEMOSHOW_CANVAS_CC(canvas, device));

	if (canvas->event == 0)
		nemotale_node_set_pick_type(canvas->node, NEMOTALE_PICK_NO_TYPE);
	else
		nemotale_node_set_id(canvas->node, canvas->event);

	return 0;
}

static inline void nemoshow_canvas_draw_item(struct showcanvas *canvas, int type, struct showitem *item, struct showitem *style)
{
	if (type == NEMOSHOW_RECT_ITEM) {
		SkRect rect = SkRect::MakeXYWH(item->x, item->y, item->width, item->height);

		if (style->fill != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRect(rect, *NEMOSHOW_ITEM_CC(style, fill));
		if (style->stroke != 0)
			NEMOSHOW_CANVAS_CC(canvas, canvas)->drawRect(rect, *NEMOSHOW_ITEM_CC(style, stroke));
	}
}

static int nemoshow_canvas_update_vector(struct nemoshow *show, struct showcanvas *canvas)
{
	int i;

	for (i = 0; i < canvas->nitems; i++) {
		struct showone *one = canvas->items[i];
		struct showitem *item = NEMOSHOW_ITEM(one);
		struct showitem *style = item->stone;

		if (item->mtone != NULL) {
			NEMOSHOW_CANVAS_CC(canvas, canvas)->save();
			NEMOSHOW_CANVAS_CC(canvas, canvas)->setMatrix(*NEMOSHOW_MATRIX_CC(item->mtone, matrix));

			nemoshow_canvas_draw_item(canvas, one->sub, item, style);

			NEMOSHOW_CANVAS_CC(canvas, canvas)->restore();
		} else {
			nemoshow_canvas_draw_item(canvas, one->sub, item, style);
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
