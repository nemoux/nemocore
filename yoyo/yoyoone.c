#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyoone.h>

struct yoyoone *nemoyoyo_one_create(void)
{
	struct yoyoone *one;

	one = (struct yoyoone *)malloc(sizeof(struct yoyoone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct yoyoone));

	one->poly = nemocook_polygon_create();

	one->trans = nemocook_transform_create();
	nemocook_polygon_set_transform(one->poly, one->trans);

	one->geometry.tx = 0.0f;
	one->geometry.ty = 0.0f;
	one->geometry.sx = 1.0f;
	one->geometry.sy = 1.0f;
	one->geometry.rz = 0.0f;

	one->size = sizeof(struct yoyoone);

	nemolist_init(&one->link);

	nemosignal_init(&one->destroy_signal);

	return one;
}

void nemoyoyo_one_destroy(struct yoyoone *one)
{
	nemosignal_emit(&one->destroy_signal, one);

	nemolist_remove(&one->link);

	nemocook_transform_destroy(one->trans);
	nemocook_polygon_destroy(one->poly);

	free(one);
}

void nemoyoyo_one_update(struct yoyoone *one)
{
	if (nemoyoyo_one_has_dirty(one, NEMOYOYO_ONE_TRANSFORM_DIRTY) != 0) {
		nemocook_transform_set_translate(one->trans,
				one->geometry.tx,
				one->geometry.ty,
				0.0f);
		nemocook_transform_set_scale(one->trans,
				one->geometry.w * one->geometry.sx,
				one->geometry.h * one->geometry.sy,
				1.0f);
		nemocook_transform_set_rotate(one->trans, 0.0f, 0.0f, one->geometry.rz);
		nemocook_transform_update(one->trans);

		nemocook_polygon_update_transform(one->poly);
	}
}
