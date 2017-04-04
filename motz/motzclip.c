#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <motzclip.h>
#include <nemomisc.h>

static void nemomotz_clip_draw(struct nemomotz *motz, struct motzone *one)
{
	struct motzclip *clip = NEMOMOTZ_CLIP(one);
	struct nemotozz *tozz = motz->tozz;
	struct motzone *child;

	nemotozz_save(tozz);
	nemotozz_clip_rectangle(tozz, clip->x, clip->y, clip->w, clip->h);

	nemolist_for_each(child, &one->one_list, link) {
		nemomotz_one_draw(motz, child);
	}

	nemotozz_restore(tozz);
}

static void nemomotz_clip_down(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_clip_motion(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static void nemomotz_clip_up(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

static struct motzone *nemomotz_clip_contain(struct motzone *one, float x, float y)
{
	struct motzclip *clip = NEMOMOTZ_CLIP(one);
	struct motzone *child;
	struct motzone *tone;

	nemolist_for_each_reverse(child, &one->one_list, link) {
		tone = nemomotz_one_contain(child, x, y);
		if (tone != NULL)
			return tone;
	}

	return NULL;
}

static void nemomotz_clip_update(struct motzone *one)
{
	struct motzclip *clip = NEMOMOTZ_CLIP(one);
}

static void nemomotz_clip_destroy(struct motzone *one)
{
	struct motzclip *clip = NEMOMOTZ_CLIP(one);

	nemomotz_one_finish(one);

	free(clip);
}

struct motzone *nemomotz_clip_create(void)
{
	struct motzclip *clip;
	struct motzone *one;

	clip = (struct motzclip *)malloc(sizeof(struct motzclip));
	if (clip == NULL)
		return NULL;
	memset(clip, 0, sizeof(struct motzclip));

	one = &clip->one;

	nemomotz_one_prepare(one);

	one->draw = nemomotz_clip_draw;
	one->down = nemomotz_clip_down;
	one->motion = nemomotz_clip_motion;
	one->up = nemomotz_clip_up;
	one->contain = nemomotz_clip_contain;
	one->update = nemomotz_clip_update;
	one->destroy = nemomotz_clip_destroy;

	return one;
}
