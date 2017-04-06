#ifndef __NEMOYOYO_ACTOR_H__
#define __NEMOYOYO_ACTOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotimer.h>

struct json_object;

struct nemoyoyo;

struct yoyoactor {
	struct nemoyoyo *yoyo;

	struct nemotimer *timer;

	struct json_object *jobj;
};

extern struct yoyoactor *nemoyoyo_actor_create(struct nemoyoyo *yoyo);
extern void nemoyoyo_actor_destroy(struct yoyoactor *actor);

extern void nemoyoyo_actor_set_json_object(struct yoyoactor *actor, struct json_object *jobj);

extern void nemoyoyo_actor_dispatch(struct yoyoactor *actor);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
