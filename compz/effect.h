#ifndef	__NEMO_EFFECT_H__
#define	__NEMO_EFFECT_H__

#include <stdint.h>

struct nemoeffect;

typedef int (*nemoeffect_frame_t)(struct nemoeffect *effect, uint32_t msecs);
typedef void (*nemoeffect_done_t)(struct nemoeffect *effect);

struct nemoeffect {
	struct wl_list link;

	uint32_t frame_count;
	uint32_t stime, ptime;

	nemoeffect_frame_t frame;
	nemoeffect_done_t done;
};

#endif
