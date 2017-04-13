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

	if (actor->trans != NULL)
		nemocook_transform_destroy(actor->trans);

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
	struct nemoyoyo *yoyo = actor->yoyo;

	nemoyoyo_actor_deactivate(actor);
	nemoyoyo_actor_destroy(actor);

	nemoyoyo_dispatch_frame(yoyo);
}

int nemoyoyo_actor_dispatch(struct yoyoactor *actor, float x, float y, float r)
{
	struct nemoyoyo *yoyo = actor->yoyo;

	actor->geometry.x = x;
	actor->geometry.y = y;
	actor->geometry.r = r;

	actor->trans = nemocook_transform_create();
	nemocook_transform_set_rotate(actor->trans, 0.0f, 0.0f, r);
	nemocook_transform_update(actor->trans);

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
	struct json_object *tobj;
	const char *type;

	cobj = nemojson_array_get_object(actor->jobj, index);
	if (cobj == NULL)
		return -1;

	type = nemojson_object_get_string(cobj, "type", NULL);
	if (type == NULL)
		return -1;

	if (strcmp(type, "app") == 0 || strcmp(type, "xapp") == 0) {
		struct busmsg *msg;
		const char *path;

		path = nemojson_object_get_string(cobj, "path", NULL);

		msg = nemobus_msg_create();
		nemobus_msg_set_name(msg, "command");
		nemobus_msg_set_attr(msg, "type", type);
		nemobus_msg_set_attr_format(msg, "x", "%f", x);
		nemobus_msg_set_attr_format(msg, "y", "%f", y);
		nemobus_msg_set_attr_format(msg, "r", "%f", actor->geometry.r * 180.0f / M_PI);
		nemobus_msg_set_attr(msg, "path", path);

		tobj = nemojson_object_get_object(cobj, "param", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			char args[512] = "";
			int i;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (i = 0; i < nemojson_get_count(json); i++) {
				const char *ikey = nemojson_get_key(json, i);
				const char *istr = nemojson_get_string(json, i);

				if (strcmp(ikey, "#optind") == 0) {
					strcat(args, istr);
					strcat(args, ";");
				} else if (istr[0] != '\0' && istr[0] != ' ' && istr[0] != '\t' && istr[0] != '\n') {
					strcat(args, "--");
					strcat(args, ikey);
					strcat(args, ";");
					strcat(args, istr);
					strcat(args, ";");
				} else {
					strcat(args, "--");
					strcat(args, ikey);
					strcat(args, ";");
				}
			}

			nemojson_destroy(json);

			nemobus_msg_set_attr(msg, "args", args);
		}

		tobj = nemojson_object_get_object(cobj, "state", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			int i;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (i = 0; i < nemojson_get_count(json); i++) {
				const char *ikey = nemojson_get_key(json, i);
				const char *istr = nemojson_get_string(json, i);

				nemobus_msg_set_attr(msg, ikey, istr);
			}

			nemojson_destroy(json);
		}

		nemobus_send_msg(yoyo->bus, yoyo->busid, "/nemoshell", msg);
		nemobus_msg_destroy(msg);

		nemoyoyo_actor_deactivate(actor);
		nemoyoyo_actor_destroy(actor);
	} else if (strcmp(type, "group") == 0) {
		nemoyoyo_actor_deactivate(actor);
		nemoyoyo_actor_activate(actor,
				nemojson_object_get_object(cobj, "items", NULL));
	} else if (strcmp(type, "menu") == 0) {
		nemoyoyo_actor_deactivate(actor);

		tobj = nemojson_search_attribute(
				nemojson_search_object(yoyo->config, 0, 1, "menu"),
				"id",
				nemojson_object_get_string(cobj, "target", NULL));
		nemoyoyo_actor_activate(actor,
				nemojson_object_get_object(tobj, "items", NULL));
	} else if (strcmp(type, "command") == 0) {
		struct busmsg *msg;
		const char *path;

		path = nemojson_object_get_string(cobj, "path", NULL);

		msg = nemobus_msg_create();
		nemobus_msg_set_name(msg, "command");
		nemobus_msg_set_attr(msg, "type", path);
		nemobus_msg_set_attr_format(msg, "x", "%f", x);
		nemobus_msg_set_attr_format(msg, "y", "%f", y);
		nemobus_msg_set_attr_format(msg, "r", "%f", actor->geometry.r * 180.0f / M_PI);
		nemobus_msg_set_attr(msg, "path", path);

		tobj = nemojson_object_get_object(cobj, "option", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			int i;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (i = 0; i < nemojson_get_count(json); i++) {
				const char *ikey = nemojson_get_key(json, i);
				const char *istr = nemojson_get_string(json, i);

				nemobus_msg_set_attr(msg, ikey, istr);
			}

			nemojson_destroy(json);
		}

		nemobus_send_msg(yoyo->bus, yoyo->busid, "/nemoshell", msg);
		nemobus_msg_destroy(msg);

		nemoyoyo_actor_deactivate(actor);
		nemoyoyo_actor_destroy(actor);
	} else if (strcmp(type, "exit") == 0) {
		nemoyoyo_actor_deactivate(actor);
		nemoyoyo_actor_destroy(actor);
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
			float row = nemojson_object_get_double(cobj, "row", 0.0f);
			float column = nemojson_object_get_double(cobj, "column", 0.0f);
			float rows = nemojson_object_get_double(cobj, "rows", 1.0f);
			float columns = nemojson_object_get_double(cobj, "columns", 1.0f);
			float x = actor->itemsize * column;
			float y = actor->itemsize * row;
			float width = actor->itemsize * columns;
			float height = actor->itemsize * rows;
			float tx0, ty0;
			float tx1, ty1;

			tex = nemoyoyo_search_texture(yoyo, icon, width, height);
			if (tex == NULL)
				break;

			one = actor->ones[i] = nemoyoyo_one_create();
			nemoyoyo_one_set_flags(one, NEMOYOYO_ONE_PICK_FLAG);
			nemoyoyo_one_set_tx(one, actor->geometry.x);
			nemoyoyo_one_set_ty(one, actor->geometry.y);
			nemoyoyo_one_set_rz(one, actor->geometry.r);
			nemoyoyo_one_set_alpha(one, 0.0f);
			nemoyoyo_one_set_width(one, width);
			nemoyoyo_one_set_height(one, height);
			nemoyoyo_one_set_texture(one, tex);
			nemoyoyo_one_set_tag(one, i);
			nemoyoyo_one_set_userdata(one, actor);
			nemoyoyo_attach_one_above(yoyo, one, NULL);

			nemoaction_one_set_tap_callback(yoyo->action, one, &one->destroy_signal, nemoyoyo_actor_dispatch_tap_event);

			trans = nemotransition_create(8, NEMOEASE_CUBIC_OUT_TYPE, actor->movetime, random_get_integer(0, actor->movetime));
			nemoyoyo_one_transition_set_tx(trans, 0, one);
			nemoyoyo_one_transition_set_ty(trans, 1, one);
			nemoyoyo_one_transition_set_alpha(trans, 2, one);
			nemoyoyo_one_transition_check_destroy(trans, one);

			if (column == 0.0f || row == 0.0f) {
				nemocook_2d_transform_to_global(actor->trans, x, -y, &tx0, &ty0);

				nemotransition_set_target(trans, 0, 1.0f, actor->geometry.x + tx0);
				nemotransition_set_target(trans, 1, 1.0f, actor->geometry.y + ty0);
			} else {
				nemocook_2d_transform_to_global(actor->trans, x, 0.0f, &tx0, &ty0);
				nemocook_2d_transform_to_global(actor->trans, x, -y, &tx1, &ty1);

				nemotransition_set_target(trans, 0, 0.5f, actor->geometry.x + tx0);
				nemotransition_set_target(trans, 1, 0.5f, actor->geometry.y + ty0);
				nemotransition_set_target(trans, 0, 1.0f, actor->geometry.x + tx1);
				nemotransition_set_target(trans, 1, 1.0f, actor->geometry.y + ty1);
			}

			nemotransition_set_target(trans, 2, 1.0f, 1.0f);
			nemotransition_group_attach_transition(yoyo->transitions, trans);
		}
	}

	nemoyoyo_dispatch_frame(yoyo);

	return 0;
}

static void nemoyoyo_actor_dispatch_transition_done(struct nemotransition *trans, void *data)
{
	struct yoyoone *one = (struct yoyoone *)data;

	nemoyoyo_one_destroy(one);
}

void nemoyoyo_actor_deactivate(struct yoyoactor *actor)
{
	struct nemoyoyo *yoyo = actor->yoyo;
	struct nemotransition *trans;
	int i;

	for (i = 0; i < actor->nones; i++) {
		nemoyoyo_one_put_flags(actor->ones[i], NEMOYOYO_ONE_PICK_FLAG);
		nemoaction_destroy_tap_by_target(yoyo->action, actor->ones[i]);

		trans = nemotransition_create(8, NEMOEASE_CUBIC_OUT_TYPE, actor->hidetime, random_get_integer(0, actor->hidetime));
		nemoyoyo_one_transition_set_alpha(trans, 0, actor->ones[i]);
		nemotransition_set_target(trans, 0, 1.0f, 0.0f);
		nemotransition_set_dispatch_done(trans, nemoyoyo_actor_dispatch_transition_done);
		nemotransition_set_userdata(trans, actor->ones[i]);
		nemotransition_group_attach_transition(yoyo->transitions, trans);
	}

	free(actor->ones);
	actor->ones = NULL;
	actor->nones = 0;

	nemoyoyo_dispatch_frame(yoyo);
}
