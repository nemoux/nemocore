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
	nemoyoyo_one_destroy(actor->icon);

	if (actor->timer != NULL)
		nemotimer_destroy(actor->timer);

	free(actor);
}

void nemoyoyo_actor_set_json_object(struct yoyoactor *actor, struct json_object *jobj)
{
	actor->jobj = jobj;
}

static void nemoyoyo_actor_dispatch_transition_done(struct nemotransition *trans, void *data)
{
	struct yoyoactor *actor = (struct yoyoactor *)data;

	nemoyoyo_actor_destroy(actor);
}

static void nemoyoyo_actor_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct yoyoactor *actor = (struct yoyoactor *)data;
	struct nemoyoyo *yoyo = actor->yoyo;
	struct yoyoone *one = actor->icon;
	struct nemotransition *trans;

	trans = nemotransition_create(8,
			NEMOEASE_CUBIC_INOUT_TYPE,
			actor->hidetime,
			0);
	nemoyoyo_one_transition_set_sx(trans, 0, one);
	nemoyoyo_one_transition_set_sy(trans, 1, one);
	nemoyoyo_one_transition_set_alpha(trans, 2, one);
	nemotransition_set_target(trans, 0, 1.0f, 2.0f);
	nemotransition_set_target(trans, 1, 1.0f, 2.0f);
	nemotransition_set_target(trans, 2, 0.5f, 0.1f);
	nemotransition_set_target(trans, 2, 1.0f, 0.0f);
	nemotransition_set_dispatch_done(trans, nemoyoyo_actor_dispatch_transition_done);
	nemotransition_set_userdata(trans, actor);
	nemotransition_group_attach_transition(yoyo->transitions, trans);

	nemocanvas_dispatch_frame(yoyo->canvas);
}

static int nemoyoyo_actor_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	if (event & NEMOACTION_TAP_DOWN_EVENT) {
	} else if (event & NEMOACTION_TAP_MOTION_EVENT) {
	} else if (event & NEMOACTION_TAP_UP_EVENT) {
	}

	return 0;
}

int nemoyoyo_actor_dispatch(struct yoyoactor *actor, float x, float y)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct yoyoone *one;
	struct cooktex *tex;
	struct json_object *iobj = NULL;
	struct json_object *tobj = NULL;
	const char *icon = NULL;
	const char *type = NULL;

	if (json_object_object_get_ex(actor->jobj, "icon", &iobj) != 0) {
		icon = json_object_get_string(iobj);
	}

	if (json_object_object_get_ex(actor->jobj, "type", &tobj) != 0) {
		type = json_object_get_string(tobj);
	}

	if (icon == NULL || type == NULL)
		goto err1;

	tex = nemoyoyo_search_tex(yoyo, icon);
	if (tex == NULL)
		goto err1;

	one = actor->icon = nemoyoyo_one_create();
	nemoyoyo_one_set_tx(one, x);
	nemoyoyo_one_set_ty(one, y);
	nemoyoyo_one_set_width(one,
			nemocook_texture_get_width(tex));
	nemoyoyo_one_set_height(one,
			nemocook_texture_get_height(tex));
	nemoyoyo_one_set_texture(one, tex);
	nemoyoyo_one_set_tap_event_callback(one, nemoyoyo_actor_dispatch_tap_event);
	nemoyoyo_attach_one(yoyo, one);

	nemocanvas_dispatch_frame(yoyo->canvas);

	actor->timer = nemotimer_create(nemotool_get_instance());
	nemotimer_set_callback(actor->timer, nemoyoyo_actor_dispatch_timer);
	nemotimer_set_userdata(actor->timer, actor);

	nemotimer_set_timeout(actor->timer, actor->lifetime);

	return 0;

err1:
	nemoyoyo_actor_destroy(actor);

	return -1;
}
