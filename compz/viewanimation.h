#ifndef	__VIEW_ANIMATION_H__
#define	__VIEW_ANIMATION_H__

#include <stdint.h>

#include <animation.h>

typedef enum {
	NEMO_VIEW_TRANSLATE_ANIMATION = (1 << 0),
	NEMO_VIEW_ROTATE_ANIMATION = (1 << 1),
	NEMO_VIEW_SCALE_ANIMATION = (1 << 2)
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
};

extern struct viewanimation *viewanimation_create(struct nemoview *view, uint32_t ease, uint32_t delay, uint32_t duration);
extern struct viewanimation *viewanimation_create_cubic(struct nemoview *view, double sx, double sy, double ex, double ey, uint32_t delay, uint32_t duration);
extern void viewanimation_destroy(struct viewanimation *animation);

extern void viewanimation_dispatch(struct nemocompz *compz, struct viewanimation *animation);

extern int viewanimation_revoke(struct nemocompz *compz, struct nemoview *view);

#endif
