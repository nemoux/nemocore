#ifndef	__NEMO_EFFECT_H__
#define	__NEMO_EFFECT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
