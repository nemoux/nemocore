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

	nemoobject_set_reserved(&one->object, "type", canvas->types, NEMOSHOW_CANVAS_TYPE_MAX);
	nemoobject_set_reserved(&one->object, "src", canvas->src, NEMOSHOW_CANVAS_SRC_MAX);
	nemoobject_set_reserved(&one->object, "width", &canvas->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &canvas->height, sizeof(double));

	return one;
}

void nemoshow_canvas_destroy(struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_one_finish(one);

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
		{ "vec",				NEMOSHOW_CANVAS_VECTOR_TYPE },
		{ "svg",				NEMOSHOW_CANVAS_SVG_TYPE },
		{ "img",				NEMOSHOW_CANVAS_IMAGE_TYPE },
		{ "ref",				NEMOSHOW_CANVAS_REF_TYPE },
		{ "use",				NEMOSHOW_CANVAS_USE_TYPE },
		{ "scene",			NEMOSHOW_CANVAS_SCENE_TYPE },
		{ "opengl",			NEMOSHOW_CANVAS_OPENGL_TYPE },
		{ "pixman",			NEMOSHOW_CANVAS_PIXMAN_TYPE },
	}, *map;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	map = static_cast<struct canvasmap *>(bsearch(canvas->types, static_cast<void *>(maps), sizeof(maps) / sizeof(maps[0]), sizeof(maps[0]), nemoshow_canvas_compare));
	if (map == NULL)
		canvas->type = NEMOSHOW_CANVAS_NONE_TYPE;
	else
		canvas->type = map->type;

	canvas->node = nemotale_node_create_pixman(canvas->width, canvas->height);

	return 0;
}

int nemoshow_canvas_update(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (canvas->type == NEMOSHOW_CANVAS_VECTOR_TYPE) {
	} else if (canvas->type == NEMOSHOW_CANVAS_SVG_TYPE) {
	} else {
		nemotale_node_fill_pixman(canvas->node, 0.0f, 1.0f, 1.0f, 1.0f);
	}

	return 0;
}
