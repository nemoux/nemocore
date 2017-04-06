#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <json.h>

#include <yoyoactor.h>
#include <nemoyoyo.h>
#include <yoyoone.h>
#include <nemomisc.h>

struct yoyoactor *nemoyoyo_actor_create(struct nemoyoyo *yoyo)
{
	struct yoyoactor *actor;

	actor = (struct yoyoactor *)malloc(sizeof(struct yoyoactor));
	if (actor == NULL)
		return NULL;
	memset(actor, 0, sizeof(struct yoyoactor));

	actor->yoyo = yoyo;

	return actor;
}

void nemoyoyo_actor_destroy(struct yoyoactor *actor)
{
	if (actor->timer != NULL)
		nemotimer_destroy(actor->timer);

	free(actor);
}

void nemoyoyo_actor_set_json_object(struct yoyoactor *actor, struct json_object *jobj)
{
	actor->jobj = jobj;
}

static void nemoyoyo_actor_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct yoyoactor *actor = (struct yoyoactor *)data;

	nemoyoyo_actor_destroy(actor);
}

void nemoyoyo_actor_dispatch(struct yoyoactor *actor)
{
	struct json_object *iobj = NULL;
	struct json_object *tobj = NULL;
	const char *icon = NULL;
	const char *type = NULL;

	actor->timer = nemotimer_create(nemotool_get_instance());
	nemotimer_set_callback(actor->timer, nemoyoyo_actor_dispatch_timer);
	nemotimer_set_userdata(actor->timer, actor);

	if (json_object_object_get_ex(actor->jobj, "icon", &iobj) != 0) {
		icon = json_object_get_string(iobj);
	}

	if (json_object_object_get_ex(actor->jobj, "type", &tobj) != 0) {
		type = json_object_get_string(tobj);
	}
}
