#ifndef	__NEMOMOTZ_H__
#define	__NEMOMOTZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

struct nemomotz;
struct motzone;

typedef void (*nemomotz_one_draw_t)(struct motzone *one);
typedef void (*nemomotz_one_down_t)(struct motzone *one, float x, float y);
typedef void (*nemomotz_one_motion_t)(struct motzone *one, float x, float y);
typedef void (*nemomotz_one_up_t)(struct motzone *one, float x, float y);

struct nemomotz {
	struct nemolist one_list;
};

struct motzone {
	nemomotz_one_draw_t draw;
	nemomotz_one_down_t down;
	nemomotz_one_motion_t motion;
	nemomotz_one_up_t up;

	struct nemolist link;
};

extern struct nemomotz *nemomotz_create(void);
extern void nemomotz_destroy(struct nemomotz *motz);

extern void nemomotz_attach_one(struct nemomotz *motz, struct motzone *one);
extern void nemomotz_detach_one(struct nemomotz *motz, struct motzone *one);

extern struct motzone *nemomotz_one_create(void);
extern void nemomotz_one_destroy(struct motzone *one);

extern void nemomotz_one_draw_null(struct motzone *one);
extern void nemomotz_one_down_null(struct motzone *one, float x, float y);
extern void nemomotz_one_motion_null(struct motzone *one, float x, float y);
extern void nemomotz_one_up_null(struct motzone *one, float x, float y);

static inline void nemomotz_one_set_draw_callback(struct motzone *one, nemomotz_one_draw_t draw)
{
	if (draw != NULL)
		one->draw = draw;
	else
		one->draw = nemomotz_one_draw_null;
}

static inline void nemomotz_one_draw(struct motzone *one)
{
	one->draw(one);
}

static inline void nemomotz_one_set_down_callback(struct motzone *one, nemomotz_one_down_t down)
{
	if (down != NULL)
		one->down = down;
	else
		one->down = nemomotz_one_down_null;
}

static inline void nemomotz_one_down(struct motzone *one, float x, float y)
{
	one->down(one, x, y);
}

static inline void nemomotz_one_set_motion_callback(struct motzone *one, nemomotz_one_motion_t motion)
{
	if (motion != NULL)
		one->motion = motion;
	else
		one->motion = nemomotz_one_motion_null;
}

static inline void nemomotz_one_motion(struct motzone *one, float x, float y)
{
	one->motion(one, x, y);
}

static inline void nemomotz_one_set_up_callback(struct motzone *one, nemomotz_one_up_t up)
{
	if (up != NULL)
		one->up = up;
	else
		one->up = nemomotz_one_up_null;
}

static inline void nemomotz_one_up(struct motzone *one, float x, float y)
{
	one->up(one, x, y);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
