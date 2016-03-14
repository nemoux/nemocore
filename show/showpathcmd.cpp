#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showpathcmd.h>
#include <nemoshow.h>
#include <nemomisc.h>

struct showone *nemoshow_pathcmd_create(int type)
{
	struct showpathcmd *pcmd;
	struct showone *one;

	pcmd = (struct showpathcmd *)malloc(sizeof(struct showpathcmd));
	if (pcmd == NULL)
		return NULL;
	memset(pcmd, 0, sizeof(struct showpathcmd));

	one = &pcmd->base;
	one->type = NEMOSHOW_PATHCMD_TYPE;
	one->sub = type;
	one->update = nemoshow_pathcmd_update;
	one->destroy = nemoshow_pathcmd_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &pcmd->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &pcmd->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x0", &pcmd->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y0", &pcmd->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x1", &pcmd->x1, sizeof(double));
	nemoobject_set_reserved(&one->object, "y1", &pcmd->y1, sizeof(double));
	nemoobject_set_reserved(&one->object, "x2", &pcmd->x2, sizeof(double));
	nemoobject_set_reserved(&one->object, "y2", &pcmd->y2, sizeof(double));

	nemoshow_one_set_state(one, NEMOSHOW_EFFECT_STATE);
	nemoshow_one_set_effect(one, NEMOSHOW_PATH_DIRTY);

	return one;
}

void nemoshow_pathcmd_destroy(struct showone *one)
{
	struct showpathcmd *pcmd = NEMOSHOW_PATHCMD(one);

	nemoshow_one_finish(one);

	free(pcmd);
}

int nemoshow_pathcmd_arrange(struct showone *one)
{
	return 0;
}

int nemoshow_pathcmd_update(struct showone *one)
{
	return 0;
}
