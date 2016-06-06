#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
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
	one->update = nemoshow_scene_update;
	one->destroy = nemoshow_scene_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "width", &scene->width, sizeof(double));
	nemoobject_set_reserved(&one->object, "height", &scene->height, sizeof(double));

	return one;
}

void nemoshow_scene_destroy(struct showone *one)
{
	struct showscene *scene = NEMOSHOW_SCENE(one);

	nemoshow_one_finish(one);

	free(scene);
}

int nemoshow_scene_update(struct showone *one)
{
	struct nemoshow *show = one->show;
	struct showscene *scene = NEMOSHOW_SCENE(one);
	struct showcanvas *canvas;
	struct showone *child;

	if (one->dirty & NEMOSHOW_CHILDREN_DIRTY) {
		nemotale_clear_node(show->tale);

		nemoshow_children_for_each(child, one) {
			if (child->type == NEMOSHOW_CANVAS_TYPE) {
				canvas = NEMOSHOW_CANVAS(child);

				nemotale_attach_node(show->tale, canvas->node);
			}
		}
	}

	return 0;
}

void nemoshow_scene_set_width(struct showone *one, double width)
{
	struct showscene *scene = NEMOSHOW_SCENE(one);

	scene->width = width;

	if (one->show != NULL)
		nemotale_set_width(one->show->tale, width);
}

void nemoshow_scene_set_height(struct showone *one, double height)
{
	struct showscene *scene = NEMOSHOW_SCENE(one);

	scene->height = height;

	if (one->show != NULL)
		nemotale_set_height(one->show->tale, height);
}
