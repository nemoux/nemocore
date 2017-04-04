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
#include <nemotransition.h>
#include <nemolist.h>

struct yoyoone;

struct nemoyoyo {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemocook *cook;
	struct cookegl *egl;
	struct cookshader *shader;
	struct cooktrans *projection;
	struct nemoaction *action;

	int width, height;

	pixman_region32_t damage;

	struct nemolist one_list;

	struct transitiongroup *transitions;

	struct {
		struct cooktex **textures;
		int ntextures;
	} spot;
};

extern void nemoyoyo_attach_one(struct nemoyoyo *yoyo, struct yoyoone *one);
extern void nemoyoyo_detach_one(struct nemoyoyo *yoyo, struct yoyoone *one);

static inline void nemoyoyo_damage(struct nemoyoyo *yoyo, pixman_region32_t *damage)
{
	pixman_region32_union(&yoyo->damage, &yoyo->damage, damage);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
