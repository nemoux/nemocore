#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showpath.h>
#include <nemoshow.h>
#include <svghelper.h>
#include <nemomisc.h>

struct showone *nemoshow_path_create(int type)
{
	struct showpath *path;
	struct showone *one;

	path = (struct showpath *)malloc(sizeof(struct showpath));
	if (path == NULL)
		return NULL;
	memset(path, 0, sizeof(struct showpath));

	one = &path->base;
	one->type = NEMOSHOW_PATH_TYPE;
	one->sub = type;
	one->update = nemoshow_path_update;
	one->destroy = nemoshow_path_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &path->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &path->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x0", &path->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y0", &path->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x1", &path->x1, sizeof(double));
	nemoobject_set_reserved(&one->object, "y1", &path->y1, sizeof(double));
	nemoobject_set_reserved(&one->object, "x2", &path->x2, sizeof(double));
	nemoobject_set_reserved(&one->object, "y2", &path->y2, sizeof(double));
	nemoobject_set_reserved(&one->object, "width", &path->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &path->height, sizeof(double));
	nemoobject_set_reserved(&one->object, "r", &path->r, sizeof(double));

	return one;
}

void nemoshow_path_destroy(struct showone *one)
{
	struct showpath *path = NEMOSHOW_PATH(one);

	nemoshow_one_finish(one);

	free(path);
}

int nemoshow_path_arrange(struct nemoshow *show, struct showone *one)
{
	if (one->sub == NEMOSHOW_SVG_PATH) {
		nemoshow_svg_load_uri_path(show, one, nemoobject_gets(&one->object, "uri"));
	}

	return 0;
}

int nemoshow_path_update(struct nemoshow *show, struct showone *one)
{
	return 0;
}
