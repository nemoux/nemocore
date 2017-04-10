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

	nemolist_init(&actor->link);

	return actor;
}

void nemoyoyo_actor_destroy(struct yoyoactor *actor)
{
	int i;

	nemolist_remove(&actor->link);

	if (actor->timer != NULL)
		nemotimer_destroy(actor->timer);

	for (i = 0; i < actor->nones; i++)
		nemoyoyo_one_destroy(actor->ones[i]);

	if (actor->ones != NULL)
		free(actor->ones);

	free(actor);
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
		nemoyoyo_actor_execute(actor,
				nemoyoyo_one_get_tag(one),
				nemoaction_tap_get_tx(tap),
				nemoaction_tap_get_ty(tap),
				"click");
	}

	return 0;
}

static void nemoyoyo_actor_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct yoyoactor *actor = (struct yoyoactor *)data;

	nemoyoyo_actor_destroy(actor);
}

int nemoyoyo_actor_dispatch(struct yoyoactor *actor, float x, float y, float r)
{
	struct nemoyoyo *yoyo = actor->yoyo;

	actor->geometry.x = x;
	actor->geometry.y = y;
	actor->geometry.r = r;

	actor->timer = nemotimer_create(nemotool_get_instance());
	nemotimer_set_callback(actor->timer, nemoyoyo_actor_dispatch_timer);
	nemotimer_set_userdata(actor->timer, actor);
	nemotimer_set_timeout(actor->timer, actor->lifetime);

	nemoyoyo_attach_actor(yoyo, actor);

	return 0;
}

int nemoyoyo_actor_execute(struct yoyoactor *actor, int index, float x, float y, const char *event)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct json_object *cobj;
	const char *type;

	cobj = nemojson_array_get_object(actor->jobj, index);
	if (cobj == NULL)
		return -1;

	type = nemojson_object_get_string(cobj, "type", NULL);
	if (type == NULL)
		return -1;

	if (strcmp(type, "app") == 0) {
		struct busmsg *msg;
		const char *path;
		const char *args;

		path = nemojson_object_get_string(cobj, "path", NULL);
		args = nemojson_object_get_string(cobj, "args", NULL);

		msg = nemobus_msg_create();
		nemobus_msg_set_name(msg, "command");
		nemobus_msg_set_attr(msg, "type", type);
		nemobus_msg_set_attr_format(msg, "x", "%f", x);
		nemobus_msg_set_attr_format(msg, "y", "%f", y);
		nemobus_msg_set_attr_format(msg, "r", "%f", actor->geometry.r);
		nemobus_msg_set_attr(msg, "path", path);
		if (args != NULL)
			nemobus_msg_set_attr(msg, "args", args);
		nemobus_send_msg(yoyo->bus, yoyo->busid, "/nemoshell", msg);
		nemobus_msg_destroy(msg);
	} else if (strcmp(type, "group") == 0) {
		nemoyoyo_actor_deactivate(actor);
		nemoyoyo_actor_activate(actor,
				nemojson_object_get_object(cobj, "items", NULL));
	}

	return 0;
}

int nemoyoyo_actor_activate(struct yoyoactor *actor, struct json_object *jobj)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct json_object *cobj;
	struct yoyoone *one;
	struct cooktex *tex;
	struct nemotransition *trans;
	int i;

	if (jobj == NULL)
		return -1;
	actor->jobj = jobj;

	actor->ones = (struct yoyoone **)malloc(sizeof(struct yoyoone *) * nemojson_array_get_length(jobj));
	actor->nones = nemojson_array_get_length(jobj);

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		cobj = nemojson_array_get_object(jobj, i);
		if (cobj != NULL) {
			const char *icon = nemojson_object_get_string(cobj, "icon", NULL);
			const char *type = nemojson_object_get_string(cobj, "type", "app");
			const char *path = nemojson_object_get_string(cobj, "path", NULL);
			int row = nemojson_object_get_integer(cobj, "row", 0);
			int column = nemojson_object_get_integer(cobj, "column", 0);

			tex = nemoyoyo_search_texture(yoyo, icon, actor->itemsize, actor->itemsize);
			if (tex == NULL)
				break;

			one = actor->ones[i] = nemoyoyo_one_create();
			nemoyoyo_one_set_flags(one, NEMOYOYO_ONE_PICK_FLAG);
			nemoyoyo_one_set_tx(one, actor->geometry.x);
			nemoyoyo_one_set_ty(one, actor->geometry.y);
			nemoyoyo_one_set_rz(one, actor->geometry.r);
			nemoyoyo_one_set_alpha(one, 0.0f);
			nemoyoyo_one_set_width(one, actor->itemsize);
			nemoyoyo_one_set_height(one, actor->itemsize);
			nemoyoyo_one_set_texture(one, tex);
			nemoyoyo_one_set_tag(one, i);
			nemoyoyo_one_set_userdata(one, actor);
			nemoyoyo_attach_one(yoyo, one);

			nemoaction_one_set_tap_callback(yoyo->action, one, &one->destroy_signal, nemoyoyo_actor_dispatch_tap_event);

			trans = nemotransition_create(8, NEMOEASE_CUBIC_OUT_TYPE, actor->movetime, actor->movetime * i * 0.25f);
			nemoyoyo_one_transition_set_tx(trans, 0, one);
			nemoyoyo_one_transition_set_ty(trans, 1, one);
			nemoyoyo_one_transition_set_alpha(trans, 2, one);
			nemoyoyo_one_transition_check_destroy(trans, one);

			if (column == 0 || row == 0) {
				nemotransition_set_target(trans, 0, 1.0f,
						actor->geometry.x + column * actor->itemsize);
				nemotransition_set_target(trans, 1, 1.0f,
						actor->geometry.y - row * actor->itemsize);
			} else {
				nemotransition_set_target(trans, 0, 0.5f,
						actor->geometry.x + column * actor->itemsize);
				nemotransition_set_target(trans, 1, 0.5f,
						actor->geometry.y);
				nemotransition_set_target(trans, 0, 1.0f,
						actor->geometry.x + column * actor->itemsize);
				nemotransition_set_target(trans, 1, 1.0f,
						actor->geometry.y - row * actor->itemsize);
			}

			nemotransition_set_target(trans, 2, 1.0f, 1.0f);
			nemotransition_group_attach_transition(yoyo->transitions, trans);
		}
	}

	nemocanvas_dispatch_frame(yoyo->canvas);

	return 0;
}

void nemoyoyo_actor_deactivate(struct yoyoactor *actor)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	int i;

	for (i = 0; i < actor->nones; i++)
		nemoyoyo_one_destroy(actor->ones[i]);

	free(actor->ones);
	actor->ones = NULL;

	actor->nones = 0;

	nemocanvas_dispatch_frame(yoyo->canvas);
}
