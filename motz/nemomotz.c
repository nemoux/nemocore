#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomotz.h>
#include <nemomisc.h>

struct nemomotz *nemomotz_create(void)
{
	struct nemomotz *motz;

	motz = (struct nemomotz *)malloc(sizeof(struct nemomotz));
	if (motz == NULL)
		return NULL;
	memset(motz, 0, sizeof(struct nemomotz));

	motz->toyz = nemotoyz_create();
	if (motz->toyz == NULL)
		goto err1;

	nemolist_init(&motz->one_list);

	return motz;

err1:
	free(motz);

	return NULL;
}

void nemomotz_destroy(struct nemomotz *motz)
{
	nemolist_remove(&motz->one_list);

	nemotoyz_destroy(motz->toyz);

	free(motz);
}

void nemomotz_set_size(struct nemomotz *motz, int width, int height)
{
	motz->width = width;
	motz->height = height;
}

void nemomotz_update(struct nemomotz *motz)
{
	struct motzone *one;

	nemolist_for_each(one, &motz->one_list, link)
		nemomotz_one_update(one);
}

int nemomotz_attach_buffer(struct nemomotz *motz, void *buffer, int width, int height)
{
	motz->viewport.width = width;
	motz->viewport.height = height;

	return nemotoyz_attach_buffer(motz->toyz,
			NEMOTOYZ_CANVAS_RGBA_COLOR,
			NEMOTOYZ_CANVAS_PREMUL_ALPHA,
			buffer,
			width,
			height);
}

void nemomotz_detach_buffer(struct nemomotz *motz)
{
	nemotoyz_detach_buffer(motz->toyz);
}

void nemomotz_update_buffer(struct nemomotz *motz)
{
	struct nemotoyz *toyz = motz->toyz;
	struct motzone *one;

	nemotoyz_save(toyz);

	if (motz->width != motz->viewport.width || motz->height != motz->viewport.height)
		nemotoyz_scale(toyz,
				(float)motz->viewport.width / (float)motz->width,
				(float)motz->viewport.height / (float)motz->height);

	nemolist_for_each(one, &motz->one_list, link)
		nemomotz_one_draw(motz, one);

	nemotoyz_restore(toyz);
}

void nemomotz_attach_one(struct nemomotz *motz, struct motzone *one)
{
	nemolist_insert_tail(&motz->one_list, &one->link);
}

void nemomotz_detach_one(struct nemomotz *motz, struct motzone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);
}

void nemomotz_one_draw_null(struct nemomotz *motz, struct motzone *one)
{
}

void nemomotz_one_down_null(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

void nemomotz_one_motion_null(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

void nemomotz_one_up_null(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

void nemomotz_one_update_null(struct motzone *one)
{
}

void nemomotz_one_destroy_null(struct motzone *one)
{
	nemolist_remove(&one->link);
	nemolist_remove(&one->one_list);

	free(one);
}

struct motzone *nemomotz_one_create(void)
{
	struct motzone *one;

	one = (struct motzone *)malloc(sizeof(struct motzone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct motzone));

	nemolist_init(&one->link);
	nemolist_init(&one->one_list);

	one->draw = nemomotz_one_draw_null;
	one->down = nemomotz_one_down_null;
	one->motion = nemomotz_one_motion_null;
	one->up = nemomotz_one_up_null;
	one->update = nemomotz_one_update_null;
	one->destroy = nemomotz_one_destroy_null;

	return one;
}

int nemomotz_one_prepare(struct motzone *one)
{
	nemolist_init(&one->link);
	nemolist_init(&one->one_list);

	one->draw = nemomotz_one_draw_null;
	one->down = nemomotz_one_down_null;
	one->motion = nemomotz_one_motion_null;
	one->up = nemomotz_one_up_null;
	one->update = nemomotz_one_update_null;
	one->destroy = nemomotz_one_destroy_null;

	return 0;
}

void nemomotz_one_finish(struct motzone *one)
{
	nemolist_remove(&one->link);
	nemolist_remove(&one->one_list);
}

void nemomotz_one_attach_one(struct motzone *one, struct motzone *child)
{
	nemolist_insert_tail(&one->one_list, &child->link);
}

void nemomotz_one_detach_one(struct motzone *one, struct motzone *child)
{
	nemolist_remove(&child->link);
	nemolist_init(&child->link);
}
