#ifndef	__NEMO_ANIMATION_H__
#define	__NEMO_ANIMATION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoease.h>

struct nemoanimation;

typedef void (*nemoanimation_frame_t)(struct nemoanimation *animation, double progress);
typedef void (*nemoanimation_done_t)(struct nemoanimation *animation);

struct nemoanimation {
	struct wl_list link;

	uint32_t frame_count;

	uint32_t stime, etime;
	uint32_t delay;
	uint32_t duration;

	struct nemoease ease;

	nemoanimation_frame_t frame;
	nemoanimation_done_t done;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
