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

static void nemoenvs_handle_nemotheme_service(struct nemoenvs *envs, struct json_object *jobj)
{
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		struct json_object *tobj;
		struct nemomemo *args;
		struct nemomemo *envp;
		struct nemomemo *states;
		char binpath[256] = "";
		const char *path;

		path = nemojson_object_get_string(cobj, "path", NULL);
		if (path != NULL) {
			strcpy(binpath, path);
		} else {
			const char *pkgpath = env_get_string("NEMO_PACKAGES_PATH", NEMOUX_INSTALL_PACKAGES);
			const char *pkgname;

			pkgname = nemojson_object_get_string(cobj, "pkgname", NULL);
			if (pkgname != NULL) {
				strcpy(binpath, pkgpath);
				strncat(binpath, "/", 1);
				strcat(binpath, pkgname);
				strncat(binpath, "/", 1);
				strcat(binpath, "exec");
			}
		}

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

		envp = nemomemo_create(512);

		tobj = nemojson_object_get_object(cobj, "environ", NULL);
		if (tobj != NULL) {
			struct nemojson *json;
			int j;

			json = nemojson_create();
			nemojson_iterate_object(json, tobj);

			for (j = 0; j < nemojson_get_count(json); j++) {
				const char *ikey = nemojson_get_key(json, j);
				const char *istr = nemojson_get_string(json, j);

				nemomemo_append_format(envp, "%s=%s;", ikey, istr);
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

				nemomemo_append_format(states, "%s;%s;", ikey, istr);
			}

			nemojson_destroy(json);
		}

		nemoenvs_attach_service(envs,
				nemojson_object_get_string(cobj, "group", "background"),
				binpath,
				nemomemo_get(args),
				nemomemo_get(envp),
				nemomemo_get(states));

		nemomemo_destroy(args);
		nemomemo_destroy(envp);
		nemomemo_destroy(states);
	}
}

static void nemoenvs_handle_nemotheme_layer(struct nemoenvs *envs, struct json_object *jobj)
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

int nemoenvs_handle_json_theme(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemojson *json;
	int i;

	json = nemojson_create();
	nemojson_iterate_object(json, jobj);

	for (i = 0; i < nemojson_get_count(json); i++) {
		struct json_object *iobj = nemojson_get_object(json, i);
		const char *ikey = nemojson_get_key(json, i);

		if (strcmp(ikey, "services") == 0) {
			nemoenvs_handle_nemotheme_service(envs, iobj);
		} else if (strcmp(ikey, "layers") == 0) {
			nemoenvs_handle_nemotheme_layer(envs, iobj);
		} else if (strcmp(ikey, "defaultLayerId") == 0) {
			const char *layer;

			layer = json_object_get_string(iobj);
			if (layer != NULL)
				nemoshell_set_default_layer(shell, layer);
		} else if (strcmp(ikey, "fullscreenLayerId") == 0) {
			const char *layer;

			layer = json_object_get_string(iobj);
			if (layer != NULL)
				nemoshell_set_fullscreen_layer(shell, layer);
		}
	}

	nemojson_destroy(json);

	return 0;
}
