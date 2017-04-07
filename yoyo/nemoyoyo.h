#ifndef __NEMOYOYO_H__
#define __NEMOYOYO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pixman.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>

#include <nemocook.h>
#include <nemoaction.h>
#include <nemobus.h>
#include <nemotransition.h>
#include <nemojson.h>
#include <nemodick.h>
#include <nemolist.h>

typedef enum {
	NEMOYOYO_REDRAW_FLAG = (1 << 0)
} NemoYoyoFlag;

struct yoyoone;
struct yoyoactor;

struct nemoyoyo {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemocook *cook;
	struct cookegl *egl;
	struct cookshader *shader;
	struct cooktrans *projection;
	struct nemoaction *action;
	struct nemojson *config;
	struct nemobus *bus;
	char *busid;

	int width, height;

	uint32_t flags;

	pixman_region32_t damage;

	struct nemolist one_list;
	struct nemolist actor_list;

	struct nemodick *textures;

	struct transitiongroup *transitions;

	struct cooktex **sweeps;
	int nsweeps;
};

extern void nemoyoyo_attach_one(struct nemoyoyo *yoyo, struct yoyoone *one);
extern void nemoyoyo_detach_one(struct nemoyoyo *yoyo, struct yoyoone *one);
extern struct yoyoone *nemoyoyo_pick_one(struct nemoyoyo *yoyo, float x, float y);

extern void nemoyoyo_attach_actor(struct nemoyoyo *yoyo, struct yoyoactor *actor);
extern void nemoyoyo_detach_actor(struct nemoyoyo *yoyo, struct yoyoactor *actor);

extern struct cooktex *nemoyoyo_search_tex(struct nemoyoyo *yoyo, const char *path);

static inline void nemoyoyo_set_flags(struct nemoyoyo *yoyo, uint32_t flags)
{
	yoyo->flags |= flags;
}

static inline void nemoyoyo_put_flags(struct nemoyoyo *yoyo, uint32_t flags)
{
	yoyo->flags &= ~flags;
}

static inline int nemoyoyo_has_flags(struct nemoyoyo *yoyo, uint32_t flags)
{
	return yoyo->flags & flags;
}

static inline int nemoyoyo_has_flags_all(struct nemoyoyo *yoyo, uint32_t flags)
{
	return (yoyo->flags & flags) == flags;
}

static inline void nemoyoyo_damage(struct nemoyoyo *yoyo, pixman_region32_t *damage)
{
	pixman_region32_union(&yoyo->damage, &yoyo->damage, damage);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
