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
	if (actor->icon != NULL)
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
			NEMOEASE_CUBIC_OUT_TYPE,
			actor->hidetime,
			0);
	nemoyoyo_one_transition_set_sx(trans, 0, one);
	nemoyoyo_one_transition_set_sy(trans, 1, one);
	nemoyoyo_one_transition_set_alpha(trans, 2, one);
	nemoyoyo_one_transition_check_destroy(trans, one);
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
	struct yoyoone *one = (struct yoyoone *)nemoaction_tap_get_userdata(tap);
	struct yoyoactor *actor = (struct yoyoactor *)nemoyoyo_one_get_userdata(one);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		nemotimer_set_timeout(actor->timer, actor->lifetime);
	} else if (event & NEMOACTION_TAP_MOTION_EVENT) {
		nemotimer_set_timeout(actor->timer, actor->lifetime);
	} else if (event & NEMOACTION_TAP_UP_EVENT) {
		nemoaction_destroy_one_by_target(action, one);
		nemoaction_destroy_tap_by_target(action, one);
		nemoyoyo_actor_execute(actor,
				nemoaction_tap_get_tx(tap),
				nemoaction_tap_get_ty(tap),
				0.0f,
				"click");
		nemoyoyo_actor_destroy(actor);
	}

	return 0;
}

int nemoyoyo_actor_dispatch(struct yoyoactor *actor, float cx, float cy, float tx, float ty)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct yoyoone *one;
	struct cooktex *tex;
	struct nemotransition *trans;
	struct json_object *tobj;
	const char *icon = NULL;

	if (json_object_object_get_ex(actor->jobj, "icon", &tobj) != 0)
		icon = json_object_get_string(tobj);
	if (icon == NULL)
		goto err1;

	tex = nemoyoyo_search_tex(yoyo, icon);
	if (tex == NULL)
		goto err1;

	one = actor->icon = nemoyoyo_one_create();
	nemoyoyo_one_set_tx(one, cx);
	nemoyoyo_one_set_ty(one, cy);
	nemoyoyo_one_set_alpha(one, 0.0f);
	nemoyoyo_one_set_width(one,
			nemocook_texture_get_width(tex));
	nemoyoyo_one_set_height(one,
			nemocook_texture_get_height(tex));
	nemoyoyo_one_set_texture(one, tex);
	nemoyoyo_one_set_userdata(one, actor);
	nemoyoyo_attach_one(yoyo, one);

	nemoaction_one_set_tap_callback(yoyo->action, one, nemoyoyo_actor_dispatch_tap_event);

	trans = nemotransition_create(8,
			NEMOEASE_CUBIC_OUT_TYPE,
			actor->hidetime,
			0);
	nemoyoyo_one_transition_set_tx(trans, 0, one);
	nemoyoyo_one_transition_set_ty(trans, 1, one);
	nemoyoyo_one_transition_set_alpha(trans, 2, one);
	nemoyoyo_one_transition_check_destroy(trans, one);
	nemotransition_set_target(trans, 0, 1.0f, tx);
	nemotransition_set_target(trans, 1, 1.0f, ty);
	nemotransition_set_target(trans, 2, 1.0f, 1.0f);
	nemotransition_set_userdata(trans, actor);
	nemotransition_group_attach_transition(yoyo->transitions, trans);

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

int nemoyoyo_actor_execute(struct yoyoactor *actor, float x, float y, float r, const char *event)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct json_object *tobj;
	const char *type = NULL;

	if (json_object_object_get_ex(actor->jobj, "type", &tobj) != 0)
		type = json_object_get_string(tobj);
	if (type == NULL)
		return -1;

	if (strcmp(type, "app") == 0) {
		const char *path = NULL;
		const char *args = NULL;
		struct busmsg *msg;

		if (json_object_object_get_ex(actor->jobj, "path", &tobj) != 0)
			path = json_object_get_string(tobj);
		if (json_object_object_get_ex(actor->jobj, "args", &tobj) != 0)
			args = json_object_get_string(tobj);

		msg = nemobus_msg_create();
		nemobus_msg_set_name(msg, "command");
		nemobus_msg_set_attr(msg, "type", type);
		nemobus_msg_set_attr_format(msg, "x", "%f", x);
		nemobus_msg_set_attr_format(msg, "y", "%f", y);
		nemobus_msg_set_attr_format(msg, "r", "%f", r);
		nemobus_msg_set_attr(msg, "path", path);
		if (args != NULL)
			nemobus_msg_set_attr(msg, "args", args);
		nemobus_send_msg(yoyo->bus, yoyo->busid, "/nemoshell", msg);
		nemobus_msg_destroy(msg);
	} else if (strcmp(type, "group") == 0) {
		struct json_object *jobj;

		if (json_object_object_get_ex(actor->jobj, "item", &jobj) != 0) {
			struct yoyoactor *child;
			struct json_object *cobj;
			float cx = x;
			float cy = y;
			float r, w;
			int i;

			for (i = 0; i < json_object_array_length(jobj); i++) {
				cobj = json_object_array_get_idx(jobj, i);

				r = random_get_double(160.0f, 180.0f);
				w = random_get_double(0.0f, M_PI * 2.0f);

				child = nemoyoyo_actor_create(yoyo);
				nemoyoyo_actor_set_json_object(child, cobj);
				nemoyoyo_actor_set_lifetime(child, 1800);
				nemoyoyo_actor_set_hidetime(child, 800);
				nemoyoyo_actor_dispatch(child,
						cx,
						cy,
						cx + cos(w) * r,
						cy + sin(w) * r);
			}
		}
	}

	return 0;
}
