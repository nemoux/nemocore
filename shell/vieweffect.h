#ifndef	__NEMO_VIEW_EFFECT_H__
#define	__NEMO_VIEW_EFFECT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <effect.h>

typedef enum {
	NEMOVIEW_PITCH_EFFECT = (1 << 0),
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

	void *userdata;
};

extern struct vieweffect *vieweffect_create(struct nemoview *view);
extern void vieweffect_destroy(struct vieweffect *effect);

extern void vieweffect_dispatch(struct nemocompz *compz, struct vieweffect *effect);

extern int vieweffect_revoke(struct nemocompz *compz, struct nemoview *view);

static inline void vieweffect_set_dispatch_done(struct vieweffect *effect, nemoeffect_done_t dispatch)
{
	effect->base.done = dispatch;
}

static inline void vieweffect_set_userdata(struct vieweffect *effect, void *data)
{
	effect->userdata = data;
}

static inline void *vieweffect_get_userdata(struct vieweffect *effect)
{
	return effect->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
