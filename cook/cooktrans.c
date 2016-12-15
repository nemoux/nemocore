#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cooktrans.h>
#include <nemomisc.h>

struct cooktrans *nemocook_transform_create(void)
{
	struct cooktrans *trans;

	trans = (struct cooktrans *)malloc(sizeof(struct cooktrans));
	if (trans == NULL)
		return NULL;
	memset(trans, 0, sizeof(struct cooktrans));

	trans->tx = 0.0f;
	trans->ty = 0.0f;
	trans->tz = 0.0f;
	trans->rx = 0.0f;
	trans->ry = 0.0f;
	trans->rz = 0.0f;
	trans->sx = 1.0f;
	trans->sy = 1.0f;
	trans->sz = 1.0f;

	nemomatrix_init_identity(&trans->matrix);

	return trans;
}

void nemocook_transform_destroy(struct cooktrans *trans)
{
	free(trans);
}

int nemocook_transform_update(struct cooktrans *trans)
{
	nemomatrix_init_identity(&trans->matrix);
	nemomatrix_scale_xyz(&trans->matrix, trans->sx, trans->sy, trans->sz);
	nemomatrix_rotate_x(&trans->matrix, cos(trans->rx), sin(trans->rx));
	nemomatrix_rotate_y(&trans->matrix, cos(trans->ry), sin(trans->ry));
	nemomatrix_rotate_z(&trans->matrix, cos(trans->rz), sin(trans->rz));
	nemomatrix_translate_xyz(&trans->matrix, trans->tx, trans->ty, trans->tz);

	return 0;
}
