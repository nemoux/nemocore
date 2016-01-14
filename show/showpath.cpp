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
	one->attach = nemoshow_path_attach_one;
	one->detach = nemoshow_path_detach_one;

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

void nemoshow_path_attach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_attach_one(parent, one);

	nemoshow_one_reference_one(parent, one, NEMOSHOW_PATH_DIRTY, -1);
}

void nemoshow_path_detach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_detach_one(parent, one);
}

int nemoshow_path_arrange(struct showone *one)
{
	return 0;
}

int nemoshow_path_update(struct showone *one)
{
	return 0;
}
