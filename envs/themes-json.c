#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-xdg-shell-server-protocol.h>

#include <json.h>

#include <shell.h>
#include <compz.h>
#include <canvas.h>
#include <subcanvas.h>
#include <view.h>
#include <seat.h>
#include <keyboard.h>
#include <pointer.h>
#include <touch.h>
#include <keypad.h>
#include <datadevice.h>
#include <screen.h>
#include <plugin.h>
#include <viewanimation.h>
#include <busycursor.h>
#include <vieweffect.h>
#include <tuio.h>
#include <virtuio.h>
#include <session.h>
#include <evdevnode.h>
#include <waylandshell.h>
#include <xdgshell.h>
#include <nemoshell.h>
#include <syshelper.h>
#include <nemotoken.h>
#include <nemomemo.h>
#include <nemoitem.h>
#include <nemojson.h>
#include <nemomisc.h>
#include <nemolog.h>

#include <nemoenvs.h>

static void nemoenvs_handle_set_nemotheme_service(struct nemoenvs *envs, struct json_object *jobj, const char *type)
{
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		struct json_object *tobj;
		struct nemomemo *args;
		struct nemomemo *states;
		const char *path = NULL;

		path = nemojson_object_get_string(cobj, "path", NULL);

		args = nemomemo_create(512);
		nemomemo_append_format(args, "--width;%d;", nemojson_object_get_integer(cobj, "width", 0));
		nemomemo_append_format(args, "--height;%d;", nemojson_object_get_integer(cobj, "height", 0));
		nemomemo_append_format(args, "--layer;%s;", nemojson_object_get_string(cobj, "layerId", "background"));

		tobj = nemojson_object_get_object(cobj, "param", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			int j;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (j = 0; j < nemojson_get_count(json); j++) {
				const char *ikey = nemojson_get_key(json, j);
				const char *istr = nemojson_get_string(json, j);

				if (strcmp(ikey, "#optind") == 0) {
					nemomemo_append_format(args, "%s;", istr);
				} else if (istr[0] != '\0' && istr[0] != ' ' && istr[0] != '\t' && istr[0] != '\n') {
					nemomemo_append_format(args, "--%s;%s;", ikey, istr);
				} else {
					nemomemo_append_format(args, "--%s;", ikey);
				}
			}

			nemojson_destroy(json);
		}

		states = nemomemo_create(512);
		nemomemo_append_format(states, "%s;%f;", "x", nemojson_object_get_double(cobj, "x", 0.0f));
		nemomemo_append_format(states, "%s;%f;", "y", nemojson_object_get_double(cobj, "y", 0.0f));
		nemomemo_append_format(states, "%s;%f;", "width", nemojson_object_get_double(cobj, "width", 0.0f));
		nemomemo_append_format(states, "%s;%f;", "height", nemojson_object_get_double(cobj, "height", 0.0f));
		nemomemo_append_format(states, "%s;%f;", "dx", nemojson_object_get_double(cobj, "dx", 0.0f));
		nemomemo_append_format(states, "%s;%f;", "dy", nemojson_object_get_double(cobj, "dy", 0.0f));

		tobj = nemojson_object_get_object(cobj, "state", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			int j;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (j = 0; j < nemojson_get_count(json); j++) {
				const char *ikey = nemojson_get_key(json, j);
				const char *istr = nemojson_get_string(json, j);

				nemomemo_append_format(states, "%s:%s;", ikey, istr);
			}

			nemojson_destroy(json);
		}

		nemoenvs_attach_service(envs,
				type,
				path,
				nemomemo_get(args),
				nemomemo_get(states));

		nemomemo_destroy(args);
		nemomemo_destroy(states);
	}
}

static void nemoenvs_handle_set_nemotheme_layer(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		const char *id = nemojson_object_get_string(cobj, "id", NULL);

		if (id != NULL) {
			const char *below = nemojson_object_get_string(cobj, "below", NULL);
			struct nemolayer *layer;

			layer = nemolayer_create(compz, id);

			if (below == NULL)
				nemolayer_attach_below(layer, NULL);
			else
				nemolayer_attach_below(layer,
						nemocompz_get_layer_by_name(compz, below));
		}
	}
}

int nemoenvs_set_json_theme(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemojson *json;
	struct json_object *tobj;
	struct json_object *cobj;
	int i;

	tobj = nemojson_object_get_object(jobj, "themes", NULL);
	if (tobj == NULL)
		return -1;

	cobj = nemojson_search_attribute(tobj,
			"id",
			nemojson_object_get_string(jobj, "useThemeId", NULL));
	if (cobj == NULL)
		return -1;

	json = nemojson_create();
	nemojson_iterate_object(json, cobj);

	for (i = 0; i < nemojson_get_count(json); i++) {
		struct json_object *iobj = nemojson_get_object(json, i);
		const char *ikey = nemojson_get_key(json, i);

		if (strcmp(ikey, "backgrounds") == 0) {
			nemoenvs_handle_set_nemotheme_service(envs, iobj, "background");
		} else if (strcmp(ikey, "daemons") == 0) {
			nemoenvs_handle_set_nemotheme_service(envs, iobj, "daemon");
		} else if (strcmp(ikey, "screensavers") == 0) {
			nemoenvs_handle_set_nemotheme_service(envs, iobj, "screensaver");
		} else if (strcmp(ikey, "layers") == 0) {
			nemoenvs_handle_set_nemotheme_layer(envs, iobj);
		} else if (strcmp(ikey, "defaultLayerId") == 0) {
			struct nemolayer *layer;

			layer = nemocompz_get_layer_by_name(compz, json_object_get_string(iobj));
			if (layer != NULL)
				nemoshell_set_default_layer(shell, layer);
		} else if (strcmp(ikey, "fullscreenLayerId") == 0) {
			struct nemolayer *layer;

			layer = nemocompz_get_layer_by_name(compz, json_object_get_string(iobj));
			if (layer != NULL)
				nemoshell_set_fullscreen_layer(shell, layer);
		}
	}

	nemojson_destroy(json);

	return 0;
}
