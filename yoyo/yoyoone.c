#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyoone.h>
#include <nemoyoyo.h>
#include <nemomisc.h>

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

	pixman_region32_init(&one->bounds);

	return one;
}

void nemoyoyo_one_destroy(struct yoyoone *one)
{
	nemosignal_emit(&one->destroy_signal, one);

	nemolist_remove(&one->link);

	pixman_region32_fini(&one->bounds);

	nemocook_transform_destroy(one->trans);
	nemocook_polygon_destroy(one->poly);

	free(one);
}

static inline void nemoyoyo_one_update_transform(struct yoyoone *one)
{
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

static inline void nemoyoyo_one_update_bounds(struct yoyoone *one)
{
	if (nemoyoyo_one_has_flags(one, NEMOYOYO_ONE_TRANSFORM_FLAG) == 0) {
		pixman_region32_init_rect(&one->bounds,
				one->geometry.tx, one->geometry.ty,
				one->geometry.w, one->geometry.h);
	} else {
		float minx = HUGE_VALF, miny = HUGE_VALF;
		float maxx = -HUGE_VALF, maxy = -HUGE_VALF;
		float s[4][2] = {
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f }
		};
		float tx, ty;
		float sx, sy, ex, ey;
		int i;

		for (i = 0; i < 4; i++) {
			nemocook_2d_transform_to_global(one->trans, s[i][0], s[i][1], &tx, &ty);

			if (tx < minx)
				minx = tx;
			if (tx > maxx)
				maxx = tx;
			if (ty < miny)
				miny = ty;
			if (ty > maxy)
				maxy = ty;
		}

		sx = floorf(minx);
		sy = floorf(miny);
		ex = ceilf(maxx);
		ey = ceilf(maxy);

		pixman_region32_init_rect(&one->bounds, sx, sy, ex - sx, ey - sy);
	}
}

void nemoyoyo_one_update(struct nemoyoyo *yoyo, struct yoyoone *one)
{
	if (nemoyoyo_one_has_dirty(one, NEMOYOYO_ONE_TRANSFORM_DIRTY) != 0) {
		nemoyoyo_one_update_transform(one);
		nemoyoyo_damage(yoyo, &one->bounds);
		nemoyoyo_one_update_bounds(one);
		nemoyoyo_damage(yoyo, &one->bounds);
	}

	nemoyoyo_one_put_dirty_all(one);
}
