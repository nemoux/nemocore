#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <yoyospot.h>

struct yoyospot *nemoyoyo_spot_create(void)
{
	struct yoyospot *spot;

	spot = (struct yoyospot *)malloc(sizeof(struct yoyospot));
	if (spot == NULL)
		return NULL;
	memset(spot, 0, sizeof(struct yoyospot));

	spot->poly = nemocook_polygon_create();

	spot->trans = nemocook_transform_create();
	nemocook_polygon_set_transform(spot->poly, spot->trans);

	spot->geometry.tx = 0.0f;
	spot->geometry.ty = 0.0f;
	spot->geometry.sx = 1.0f;
	spot->geometry.sy = 1.0f;
	spot->geometry.rz = 0.0f;

	nemolist_init(&spot->link);

	return spot;
}

void nemoyoyo_spot_destroy(struct yoyospot *spot)
{
	nemolist_remove(&spot->link);

	nemocook_transform_destroy(spot->trans);
	nemocook_polygon_destroy(spot->poly);

	free(spot);
}

void nemoyoyo_spot_update(struct yoyospot *spot)
{
	nemocook_transform_set_translate(spot->trans, spot->geometry.tx, spot->geometry.ty, 0.0f);
	nemocook_transform_set_scale(spot->trans, spot->geometry.sx, spot->geometry.sy, 1.0f);
	nemocook_transform_set_rotate(spot->trans, 0.0f, 0.0f, spot->geometry.rz);
	nemocook_transform_update(spot->trans);

	nemocook_polygon_update_transform(spot->poly);
}
