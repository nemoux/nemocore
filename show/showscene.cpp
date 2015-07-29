#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showscene.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_scene_create(void)
{
	struct showscene *scene;
	struct showone *one;

	scene = (struct showscene *)malloc(sizeof(struct showscene));
	if (scene == NULL)
		return NULL;
	memset(scene, 0, sizeof(struct showscene));

	one = &scene->base;
	one->type = NEMOSHOW_SCENE_TYPE;
	one->destroy = nemoshow_scene_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "width", &scene->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &scene->height, sizeof(double));

	scene->canvases = (struct showone **)malloc(sizeof(struct showone *) * 8);
	scene->ncanvases = 0;
	scene->scanvases = 8;

	return one;
}

void nemoshow_scene_destroy(struct showone *one)
{
	struct showscene *scene = NEMOSHOW_SCENE(one);

	nemoshow_one_finish(one);

	free(scene->canvases);
	free(scene);
}
