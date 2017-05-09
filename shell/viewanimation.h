#ifndef	__NEMO_VIEW_ANIMATION_H__
#define	__NEMO_VIEW_ANIMATION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <animation.h>

typedef enum {
	NEMOVIEW_TRANSLATE_ANIMATION = (1 << 0),
	NEMOVIEW_ROTATE_ANIMATION = (1 << 1),
	NEMOVIEW_SCALE_ANIMATION = (1 << 2),
	NEMOVIEW_ALPHA_ANIMATION = (1 << 3)
} NemoViewAnimationType;

struct nemocompz;
struct nemoview;

struct viewanimation {
	struct nemoanimation base;

	struct nemoview *view;

	struct wl_listener destroy_listener;

	uint32_t type;

	struct {
		struct {
			float x, y;
		} base;

		float x, y;
	} translate;

	struct {
		struct {
			float r;
		} base;

		float r;
	} rotate;

	struct {
		struct {
			float sx, sy;
		} base;

		float sx, sy;
	} scale;

	struct {
		struct {
			float v;
		} base;

		float v;
	} alpha;

	void *userdata;
};

extern struct viewanimation *viewanimation_create(struct nemoview *view, uint32_t ease, uint32_t delay, uint32_t duration);
extern struct viewanimation *viewanimation_create_cubic(struct nemoview *view, double sx, double sy, double ex, double ey, uint32_t delay, uint32_t duration);
extern void viewanimation_destroy(struct viewanimation *animation);

extern void viewanimation_dispatch(struct nemocompz *compz, struct viewanimation *animation);

extern int viewanimation_revoke(struct nemocompz *compz, struct nemoview *view, uint32_t type);

static inline void viewanimation_set_translate(struct viewanimation *animation, float x, float y)
{
	animation->type |= NEMOVIEW_TRANSLATE_ANIMATION;
	animation->translate.x = x;
	animation->translate.y = y;
}

static inline void viewanimation_set_rotate(struct viewanimation *animation, float r)
{
	animation->type |= NEMOVIEW_ROTATE_ANIMATION;
	animation->rotate.r = r;
}

static inline void viewanimation_set_scale(struct viewanimation *animation, float sx, float sy)
{
	animation->type |= NEMOVIEW_SCALE_ANIMATION;
	animation->scale.sx = sx;
	animation->scale.sy = sy;
}

static inline void viewanimation_set_alpha(struct viewanimation *animation, float v)
{
	animation->type |= NEMOVIEW_ALPHA_ANIMATION;
	animation->alpha.v = v;
}

static inline void viewanimation_set_userdata(struct viewanimation *animation, void *data)
{
	animation->userdata = data;
}

static inline void *viewanimation_get_userdata(struct viewanimation *animation)
{
	return animation->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
