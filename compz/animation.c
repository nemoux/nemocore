#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <animation.h>
#include <compz.h>
#include <nemoease.h>
#include <nemomisc.h>

struct nemoanimation *nemoanimation_create(uint32_t ease, uint32_t delay, uint32_t duration)
{
	struct nemoanimation *animation;

	animation = (struct nemoanimation *)malloc(sizeof(struct nemoanimation));
	if (animation == NULL)
		return NULL;
	memset(animation, 0, sizeof(struct nemoanimation));

	animation->delay = delay;
	animation->duration = duration;
	nemoease_set(&animation->ease, ease);

	wl_list_init(&animation->link);

	return animation;
}

void nemoanimation_destroy(struct nemoanimation *animation)
{
	wl_list_remove(&animation->link);

	free(animation);
}

int nemoanimation_revoke(struct nemocompz *compz, void *object)
{
	struct nemoanimation *anim, *next;

	wl_list_for_each_safe(anim, next, &compz->animation_list, link) {
		if (anim->object == object) {
			nemoanimation_destroy(anim);
		}
	}

	return 0;
}
