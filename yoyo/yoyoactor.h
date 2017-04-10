#ifndef __NEMOYOYO_ACTOR_H__
#define __NEMOYOYO_ACTOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <json.h>

#include <nemolist.h>
#include <nemotimer.h>
#include <nemoaction.h>

#include <nemoyoyo.h>

struct yoyoone;

struct yoyoactor {
	struct nemoyoyo *yoyo;

	struct nemotimer *timer;

	struct yoyoone **ones;
	int nones;

	struct json_object *jobj;

	uint32_t lifetime;
	uint32_t movetime;
	float itemsize;

	struct {
		float x, y;
		float r;
	} geometry;

	struct cooktrans *trans;

	struct nemolist link;
};

extern struct yoyoactor *nemoyoyo_actor_create(struct nemoyoyo *yoyo);
extern void nemoyoyo_actor_destroy(struct yoyoactor *actor);

extern int nemoyoyo_actor_dispatch(struct yoyoactor *actor, float x, float y, float r);

extern int nemoyoyo_actor_activate(struct yoyoactor *actor, struct json_object *jobj);
extern void nemoyoyo_actor_deactivate(struct yoyoactor *actor);

extern int nemoyoyo_actor_execute(struct yoyoactor *actor, int index, float x, float y, const char *event);

NEMOYOYO_DECLARE_SET_ATTRIBUTE(actor, uint32_t, lifetime, lifetime);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(actor, uint32_t, movetime, movetime);
NEMOYOYO_DECLARE_SET_ATTRIBUTE(actor, float, itemsize, itemsize);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
