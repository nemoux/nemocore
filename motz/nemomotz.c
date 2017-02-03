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

int nemomotz_attach_buffer(struct nemomotz *motz, void *buffer, int width, int height)
{
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

void nemomotz_attach_one(struct nemomotz *motz, struct motzone *one)
{
	nemolist_insert_tail(&motz->one_list, &one->link);
}

void nemomotz_detach_one(struct nemomotz *motz, struct motzone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);
}

void nemomotz_update(struct nemomotz *motz)
{
	struct motzone *one;

	nemolist_for_each(one, &motz->one_list, link) {
		nemomotz_one_update(one);

		nemomotz_one_draw(motz, one);
	}
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
}
