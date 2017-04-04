#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzgroup.h>
#include <nemomisc.h>

static void nemomotz_group_draw(struct nemomotz *motz, struct motzone *one)
{
	struct motzgroup *group = NEMOMOTZ_GROUP(one);
	struct nemotozz *tozz = motz->tozz;
	struct motzone *child;

	nemotozz_save(tozz);
	nemotozz_concat(tozz, group->matrix);

	nemolist_for_each(child, &one->one_list, link) {
		nemomotz_one_draw(motz, child);
	}

	nemotozz_restore(tozz);
}

static void nemomotz_group_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_group_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_group_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static struct motzone *nemomotz_group_contain(struct motzone *one, float x, float y)
{
	struct motzgroup *group = NEMOMOTZ_GROUP(one);
	struct motzone *child;
	struct motzone *tone;

	nemotozz_matrix_map_point(group->inverse, &x, &y);

	nemolist_for_each_reverse(child, &one->one_list, link) {
		tone = nemomotz_one_contain(child, x, y);
		if (tone != NULL)
			return tone;
	}

	return NULL;
}

static void nemomotz_group_update(struct motzone *one)
{
	struct motzgroup *group = NEMOMOTZ_GROUP(one);

	if (nemomotz_one_has_dirty(one, NEMOMOTZ_GROUP_TRANSFORM_DIRTY) != 0) {
		nemotozz_matrix_identity(group->matrix);
		nemotozz_matrix_post_rotate(group->matrix, group->rz);
		nemotozz_matrix_post_scale(group->matrix, group->sx, group->sy);
		nemotozz_matrix_post_translate(group->matrix, group->tx, group->ty);

		nemotozz_matrix_invert(group->inverse, group->matrix);
	}
}

static void nemomotz_group_destroy(struct motzone *one)
{
	struct motzgroup *group = NEMOMOTZ_GROUP(one);

	nemomotz_one_finish(one);

	nemotozz_matrix_destroy(group->matrix);
	nemotozz_matrix_destroy(group->inverse);

	free(group);
}

struct motzone *nemomotz_group_create(void)
{
	struct motzgroup *group;
	struct motzone *one;

	group = (struct motzgroup *)malloc(sizeof(struct motzgroup));
	if (group == NULL)
		return NULL;
	memset(group, 0, sizeof(struct motzgroup));

	one = &group->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_group_draw;
	one->down = nemomotz_group_down;
	one->motion = nemomotz_group_motion;
	one->up = nemomotz_group_up;
	one->contain = nemomotz_group_contain;
	one->update = nemomotz_group_update;
	one->destroy = nemomotz_group_destroy;

	group->matrix = nemotozz_matrix_create();
	group->inverse = nemotozz_matrix_create();

	group->sx = 1.0f;
	group->sy = 1.0f;

	return one;
}
