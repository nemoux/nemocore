#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzobject.h>
#include <nemomisc.h>

static void nemomotz_object_draw(struct nemomotz *motz, struct motzone *one)
{
}

static void nemomotz_object_down(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

static void nemomotz_object_motion(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

static void nemomotz_object_up(struct nemomotz *motz, struct motzone *one, float x, float y)
{
}

static void nemomotz_object_destroy(struct motzone *one)
{
	struct motzobject *object = (struct motzobject *)container_of(one, struct motzobject, one);

	nemomotz_one_finish(one);

	nemotoyz_style_destroy(object->style);

	free(object);
}

struct motzone *nemomotz_object_create(void)
{
	struct motzobject *object;
	struct motzone *one;

	object = (struct motzobject *)malloc(sizeof(struct motzobject));
	if (object == NULL)
		return NULL;
	memset(object, 0, sizeof(struct motzobject));

	one = &object->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_object_draw;
	one->down = nemomotz_object_down;
	one->motion = nemomotz_object_motion;
	one->up = nemomotz_object_up;
	one->destroy = nemomotz_object_destroy;

	object->style = nemotoyz_style_create();

	return one;
}
