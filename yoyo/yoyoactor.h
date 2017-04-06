#ifndef __NEMOYOYO_ACTOR_H__
#define __NEMOYOYO_ACTOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <json.h>

#include <nemotimer.h>
#include <nemoaction.h>

struct nemoyoyo;
struct yoyoone;

struct yoyoactor {
	struct nemoyoyo *yoyo;

	struct nemotimer *timer;

	struct yoyoone *icon;

	struct json_object *jobj;

	uint32_t lifetime;
	uint32_t hidetime;
};

extern struct yoyoactor *nemoyoyo_actor_create(struct nemoyoyo *yoyo);
extern void nemoyoyo_actor_destroy(struct yoyoactor *actor);

extern void nemoyoyo_actor_set_json_object(struct yoyoactor *actor, struct json_object *jobj);

extern int nemoyoyo_actor_dispatch(struct yoyoactor *actor, struct actiontap *tap);
extern int nemoyoyo_actor_execute(struct yoyoactor *actor, struct actiontap *tap, const char *event);

static inline void nemoyoyo_actor_set_lifetime(struct yoyoactor *actor, uint32_t lifetime)
{
	actor->lifetime = lifetime;
}

static inline void nemoyoyo_actor_set_hidetime(struct yoyoactor *actor, uint32_t hidetime)
{
	actor->hidetime = hidetime;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
