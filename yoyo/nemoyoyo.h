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
struct yoyosweep;
struct yoyoactor;
struct yoyoregion;

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
	struct nemolist region_list;

	struct nemodick *textures;

	struct transitiongroup *transitions;

	struct cooktex **sweeps;
	int nsweeps;
};

extern struct nemoyoyo *nemoyoyo_create(void);
extern void nemoyoyo_destroy(struct nemoyoyo *yoyo);

extern int nemoyoyo_load_config(struct nemoyoyo *yoyo);

extern int nemoyoyo_update_one(struct nemoyoyo *yoyo);
extern int nemoyoyo_update_frame(struct nemoyoyo *yoyo);

extern void nemoyoyo_attach_one(struct nemoyoyo *yoyo, struct yoyoone *one);
extern void nemoyoyo_detach_one(struct nemoyoyo *yoyo, struct yoyoone *one);
extern struct yoyoone *nemoyoyo_pick_one(struct nemoyoyo *yoyo, float x, float y);

extern void nemoyoyo_attach_actor(struct nemoyoyo *yoyo, struct yoyoactor *actor);
extern void nemoyoyo_detach_actor(struct nemoyoyo *yoyo, struct yoyoactor *actor);
extern int nemoyoyo_overlap_actor(struct nemoyoyo *yoyo, float x, float y);

extern void nemoyoyo_attach_region(struct nemoyoyo *yoyo, struct yoyoregion *region);
extern void nemoyoyo_detach_region(struct nemoyoyo *yoyo, struct yoyoregion *region);
extern struct yoyoregion *nemoyoyo_search_region(struct nemoyoyo *yoyo, float x, float y);

extern struct cooktex *nemoyoyo_search_tex(struct nemoyoyo *yoyo, const char *path, int width, int height);

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

#define NEMOYOYO_DECLARE_SET_ATTRIBUTE(tag, type, attr, name)	\
	static inline void nemoyoyo_##tag##_set_##name(struct yoyo##tag *tag, type name) {	\
		tag->attr = name;	\
	}
#define NEMOYOYO_DECLARE_GET_ATTRIBUTE(tag, type, attr, name)	\
	static inline type nemoyoyo_##tag##_get_##name(struct yoyo##tag *tag) {	\
		return tag->attr;	\
	}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
