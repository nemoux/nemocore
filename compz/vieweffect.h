#ifndef	__VIEW_EFFECT_H__
#define	__VIEW_EFFECT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <effect.h>

typedef enum {
	NEMO_VIEW_PITCH_EFFECT = (1 << 0),
} NemoViewEffectType;

struct nemocompz;
struct nemoview;

struct vieweffect {
	struct nemoeffect base;

	struct nemoview *view;

	struct wl_listener destroy_listener;

	uint32_t type;

	struct {
		double dx, dy;
		double velocity;
		double friction;
	} pitch;
};

extern struct vieweffect *vieweffect_create(struct nemoview *view);
extern void vieweffect_destroy(struct vieweffect *effect);

extern void vieweffect_dispatch(struct nemocompz *compz, struct vieweffect *effect);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
