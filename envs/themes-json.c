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
#include <namespacehelper.h>
#include <nemotoken.h>
#include <nemoitem.h>
#include <nemojson.h>
#include <nemomisc.h>
#include <nemolog.h>

#include <nemoenvs.h>

static void nemoenvs_handle_set_nemotheme_background(struct nemoenvs *envs, struct json_object *jobj)
{
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		struct json_object *tobj;
		struct itemone *sone;
		const char *path = NULL;
		const char *layer = NULL;
		char args[512] = "";
		char states[512] = "";

		path = nemojson_object_get_string(cobj, "path", NULL);

		tobj = nemojson_object_get_object(cobj, "param", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
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
		}

		sone = nemoitem_one_create();
		nemoitem_one_set_attr_format(sone, "x", "%f", nemojson_object_get_double(cobj, "x", 0.0f));
		nemoitem_one_set_attr_format(sone, "y", "%f", nemojson_object_get_double(cobj, "y", 0.0f));
		nemoitem_one_set_attr_format(sone, "width", "%f", nemojson_object_get_double(cobj, "width", 0.0f));
		nemoitem_one_set_attr_format(sone, "height", "%f", nemojson_object_get_double(cobj, "height", 0.0f));
		nemoitem_one_set_attr_format(sone, "dx", "%f", nemojson_object_get_double(cobj, "dx", 0.0f));
		nemoitem_one_set_attr_format(sone, "dy", "%f", nemojson_object_get_double(cobj, "dy", 0.0f));
		nemoitem_one_set_attr(sone, "layer", nemojson_object_get_string(cobj, "layerId", "background"));

		tobj = nemojson_object_get_object(cobj, "state", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			int i;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (i = 0; i < nemojson_get_count(json); i++) {
				const char *ikey = nemojson_get_key(json, i);
				const char *istr = nemojson_get_string(json, i);

				nemoitem_one_set_attr(sone, ikey, istr);
			}

			nemojson_destroy(json);
		}
		nemoitem_one_save_attrs(sone, states, ';');
		nemoitem_one_destroy(sone);

		nemoenvs_create_service(envs, "background", path, args, states);
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
			nemoenvs_handle_set_nemotheme_background(envs, iobj);
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
