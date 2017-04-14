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

static void nemoenvs_handle_set_nemoshell_screen(struct nemoshell *shell, struct json_object *jobj)
{
	struct nemocompz *compz = shell->compz;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		uint32_t nodeid = nemojson_object_get_integer(cobj, "nodeid", 0);
		uint32_t screenid = nemojson_object_get_integer(cobj, "screenid", 0);
		struct nemoscreen *screen;

		screen = nemocompz_get_screen(compz, nodeid, screenid);
		if (screen != NULL) {
			int32_t width, height;
			int32_t x, y;
			float sx, sy;
			float r;
			float px, py;
			uint32_t refresh;
			const char *transform;
			const char *scope;

			x = nemojson_object_get_integer(cobj, "x", 0);
			y = nemojson_object_get_integer(cobj, "y", 0);
			width = nemojson_object_get_integer(cobj, "width", screen->width);
			height = nemojson_object_get_integer(cobj, "height", screen->height);
			sx = nemojson_object_get_double(cobj, "sx", 1.0f);
			sy = nemojson_object_get_double(cobj, "sy", 1.0f);
			r = nemojson_object_get_double(cobj, "r", 0.0f) * M_PI / 180.0f;
			px = nemojson_object_get_double(cobj, "px", 0.0f);
			py = nemojson_object_get_double(cobj, "py", 0.0f);
			refresh = nemojson_object_get_integer(cobj, "refresh", 0);
			transform = nemojson_object_get_string(cobj, "transform", NULL);
			scope = nemojson_object_get_string(cobj, "scope", NULL);

			if (screen->width != width || screen->height != height)
				nemoscreen_switch_mode(screen, width, height, refresh);

			nemoscreen_clear_transform(screen);

			if (transform != NULL) {
				nemoscreen_set_custom(screen, transform);
			} else {
				nemoscreen_set_position(screen, x, y);
				nemoscreen_set_scale(screen, sx, sy);
				nemoscreen_set_rotation(screen, r);
				nemoscreen_set_pivot(screen, px, py);
			}

			if (scope != NULL && strcmp(scope, "off") == 0) {
				nemoscreen_put_state(screen, NEMOSCREEN_SCOPE_STATE);
			} else {
				nemoscreen_set_state(screen, NEMOSCREEN_SCOPE_STATE);
			}

			nemoscreen_set_state(screen, NEMOSCREEN_DISPLAY_STATE);
			nemoscreen_schedule_repaint(screen);

			nemocompz_scene_dirty(compz);
		}
	}
}

int nemoenvs_set_json_config(struct nemoenvs *envs, struct json_object *jobj)
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

		if (strcmp(ikey, "screens") == 0) {
		} else if (strcmp(ikey, "inputs") == 0) {
		} else if (strcmp(ikey, "fullscreens") == 0) {
		} else if (strcmp(ikey, "daemons") == 0) {
		} else if (strcmp(ikey, "window") == 0) {
		}
	}

	nemojson_destroy(json);

	return 0;
}
