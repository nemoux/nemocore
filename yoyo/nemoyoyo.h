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
#include <nemolist.h>

struct nemoyoyo {
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct nemocook *cook;
	struct cookegl *egl;
	struct cookshader *shader;

	int width, height;

	pixman_region32_t damage;

	struct nemolist spot_list;
};

static inline void nemoyoyo_damage(struct nemoyoyo *yoyo, pixman_region32_t *damage)
{
	pixman_region32_union(&yoyo->damage, &yoyo->damage, damage);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
