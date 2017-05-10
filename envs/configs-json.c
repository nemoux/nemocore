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
#include <timer.h>
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
#include <pixmanhelper.h>
#include <nemotoken.h>
#include <nemoitem.h>
#include <nemojson.h>
#include <nemomisc.h>
#include <nemolog.h>

#include <nemoenvs.h>

static void nemoenvs_handle_nemoshell_screen(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
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

			if (scope != NULL && (strcmp(scope, "off") == 0 || strcmp(scope, "false") == 0)) {
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

static void nemoenvs_handle_nemoshell_input(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct inputnode *node;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		const char *devnode = nemojson_object_get_string(cobj, "devnode", NULL);
		const char *transform;
		int32_t x, y;
		int32_t width, height;
		int32_t nodeid, screenid;
		int32_t sampling;
		float sx, sy;
		float r;
		float px, py;
		float distance;

		x = nemojson_object_get_integer(cobj, "x", 0);
		y = nemojson_object_get_integer(cobj, "y", 0);
		width = nemojson_object_get_integer(cobj, "width", screen->width);
		height = nemojson_object_get_integer(cobj, "height", screen->height);
		nodeid = nemojson_object_get_integer(cobj, "nodeid", -1);
		screenid = nemojson_object_get_integer(cobj, "screenid", -1);
		sampling = nemojson_object_get_integer(cobj, "sampling", 0);
		sx = nemojson_object_get_double(cobj, "sx", 1.0f);
		sy = nemojson_object_get_double(cobj, "sy", 1.0f);
		r = nemojson_object_get_double(cobj, "r", 0.0f) * M_PI / 180.0f;
		px = nemojson_object_get_double(cobj, "px", 0.0f);
		py = nemojson_object_get_double(cobj, "py", 0.0f);
		distance = nemojson_object_get_double(cobj, "distance", 0.0f);
		transform = nemojson_object_get_string(cobj, "transform", NULL);

		wl_list_for_each(node, &compz->input_list, link) {
			if ((devnode != NULL && strcmp(node->devnode, devnode) == 0) ||
					(devnode == NULL && nemoinput_has_state(node, NEMOINPUT_CONFIG_STATE) == 0)) {
				if (nodeid >= 0 && screenid >= 0) {
					screen = nemocompz_get_screen(compz, nodeid, screenid);
					if (node->screen != screen)
						nemoinput_set_screen(node, screen);
				} else {
					nemoinput_put_screen(node);

					nemoinput_set_size(node, width, height);

					nemoinput_clear_transform(node);

					if (transform != NULL) {
						nemoinput_set_custom(node, transform);
					} else {
						nemoinput_set_position(node, x, y);
						nemoinput_set_scale(node, sx, sy);
						nemoinput_set_rotation(node, r);
						nemoinput_set_pivot(node, px, py);
					}

					nemoinput_update_transform(node);
				}

				nemoinput_set_sampling(node, sampling);
				nemoinput_set_maximum_distance(node, distance);

				if (devnode != NULL)
					nemoinput_set_state(node, NEMOINPUT_CONFIG_STATE);
			}
		}
	}
}

static void nemoenvs_handle_nemoshell_evdev(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct inputnode *node;
	struct evdevnode *enode;
	int fd;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		const char *devpath = nemojson_object_get_string(cobj, "devpath", NULL);
		const char *transform;
		int32_t x, y;
		int32_t width, height;
		int32_t nodeid, screenid;
		int32_t sampling;
		float sx, sy;
		float r;
		float px, py;
		float distance;

		enode = nemocompz_get_evdev(compz, devpath);
		if (enode == NULL) {
			fd = nemosession_open(compz->session, devpath, O_RDWR | O_NONBLOCK);
			if (fd < 0)
				break;

			enode = evdev_create_node(compz, devpath, fd);
			if (enode == NULL) {
				nemosession_close(compz->session, fd);
				break;
			}
		}

		node = &enode->base;

		x = nemojson_object_get_integer(cobj, "x", 0);
		y = nemojson_object_get_integer(cobj, "y", 0);
		width = nemojson_object_get_integer(cobj, "width", screen->width);
		height = nemojson_object_get_integer(cobj, "height", screen->height);
		nodeid = nemojson_object_get_integer(cobj, "nodeid", -1);
		screenid = nemojson_object_get_integer(cobj, "screenid", -1);
		sampling = nemojson_object_get_integer(cobj, "sampling", 0);
		sx = nemojson_object_get_double(cobj, "sx", 1.0f);
		sy = nemojson_object_get_double(cobj, "sy", 1.0f);
		r = nemojson_object_get_double(cobj, "r", 0.0f) * M_PI / 180.0f;
		px = nemojson_object_get_double(cobj, "px", 0.0f);
		py = nemojson_object_get_double(cobj, "py", 0.0f);
		distance = nemojson_object_get_double(cobj, "distance", 0.0f);
		transform = nemojson_object_get_string(cobj, "transform", NULL);

		if (nodeid >= 0 && screenid >= 0) {
			screen = nemocompz_get_screen(compz, nodeid, screenid);
			if (node->screen != screen)
				nemoinput_set_screen(node, screen);
		} else {
			nemoinput_put_screen(node);

			nemoinput_set_size(node, width, height);

			nemoinput_clear_transform(node);

			if (transform != NULL) {
				nemoinput_set_custom(node, transform);
			} else {
				nemoinput_set_position(node, x, y);
				nemoinput_set_scale(node, sx, sy);
				nemoinput_set_rotation(node, r);
				nemoinput_set_pivot(node, px, py);
			}

			nemoinput_update_transform(node);
		}

		nemoinput_set_sampling(node, sampling);
		nemoinput_set_maximum_distance(node, distance);
		nemoinput_set_state(node, NEMOINPUT_CONFIG_STATE);
	}
}

static void nemoenvs_handle_nemoshell_fullscreen(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct shellscreen *screen;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		struct json_object *tobj;
		const char *id = nemojson_object_get_string(cobj, "id", NULL);

		screen = nemoshell_get_fullscreen(shell, id);
		if (screen != NULL) {
			const char *type = nemojson_object_get_string(cobj, "type", "normal");
			const char *focus = nemojson_object_get_string(cobj, "focus", "none");

			tobj = nemojson_object_get_object(cobj, "source", NULL);
			if (tobj != NULL) {
				screen->sx = nemojson_object_get_integer(tobj, "x", 0);
				screen->sy = nemojson_object_get_integer(tobj, "y", 0);
				screen->sw = nemojson_object_get_integer(tobj, "width", 0);
				screen->sh = nemojson_object_get_integer(tobj, "height", 0);
			}

			tobj = nemojson_object_get_object(cobj, "destination", NULL);
			if (tobj != NULL) {
				screen->dx = nemojson_object_get_integer(tobj, "x", 0);
				screen->dy = nemojson_object_get_integer(tobj, "y", 0);
				screen->dw = nemojson_object_get_integer(tobj, "width", 0);
				screen->dh = nemojson_object_get_integer(tobj, "height", 0);
				screen->dr = nemojson_object_get_integer(tobj, "rotation", 0);
				screen->has_screen = 0;
			}

			if (strcmp(type, "pick") == 0)
				screen->type = NEMOSHELL_FULLSCREEN_PICK_TYPE;
			else if (strcmp(type, "pitch") == 0)
				screen->type = NEMOSHELL_FULLSCREEN_PITCH_TYPE;
			else
				screen->type = NEMOSHELL_FULLSCREEN_NORMAL_TYPE;

			if (strcmp(focus, "all") == 0)
				screen->focus = NEMOSHELL_FULLSCREEN_ALL_FOCUS;
			else
				screen->focus = NEMOSHELL_FULLSCREEN_NONE_FOCUS;

			screen->fixed = nemojson_object_get_boolean(cobj, "fixed", 0);
			screen->target = nemojson_object_get_integer(cobj, "target", 0);
		}
	}
}

static void nemoenvs_handle_nemoshell_daemon(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
	}
}

static void nemoenvs_handle_nemoshell_stage(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct shellstage *stage;
	int i;

	for (i = 0; i < nemojson_array_get_length(jobj); i++) {
		struct json_object *cobj = nemojson_array_get_object(jobj, i);
		const char *id = nemojson_object_get_string(cobj, "id", NULL);

		stage = nemoshell_get_stage(shell, id);
		if (stage != NULL) {
			stage->sx = nemojson_object_get_integer(cobj, "x", 0);
			stage->sy = nemojson_object_get_integer(cobj, "y", 0);
			stage->sw = nemojson_object_get_integer(cobj, "width", 0);
			stage->sh = nemojson_object_get_integer(cobj, "height", 0);
			stage->dr = nemojson_object_get_integer(cobj, "rotation", 0);
		}
	}
}

static void nemoenvs_handle_nemoshell_window(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	struct json_object *tobj;

	tobj = nemojson_object_get_object(jobj, "minimum", NULL);
	if (tobj != NULL) {
		shell->bin.min_width = nemojson_object_get_integer(tobj, "width", 0);
		shell->bin.min_height = nemojson_object_get_integer(tobj, "height", 0);
	}

	tobj = nemojson_object_get_object(jobj, "maximum", NULL);
	if (tobj != NULL) {
		shell->bin.max_width = nemojson_object_get_integer(tobj, "width", 0);
		shell->bin.max_height = nemojson_object_get_integer(tobj, "height", 0);
	}

	if (shell->bin.max_width > 0 && shell->bin.max_height > 0)
		nemocompz_set_output(shell->compz, 0, 0, shell->bin.max_width, shell->bin.max_height);
}

static void nemoenvs_handle_nemoshell_pick(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;

	shell->pick.flags = NEMOSHELL_PICK_ALL_FLAGS;

	shell->pick.rotate_distance = nemojson_object_get_double(jobj, "rotateDistance", 0.0f);
	shell->pick.scale_distance = nemojson_object_get_double(jobj, "scaleDistance", 0.0f);
	shell->pick.fullscreen_scale = nemojson_object_get_double(jobj, "fullscreenScale", 0.0f);
	shell->pick.resize_interval = nemojson_object_get_double(jobj, "resizeInterval", 0.0f);

	if (nemojson_object_get_boolean(jobj, "scale", 1) == 0)
		shell->pick.flags &= ~NEMOSHELL_PICK_SCALE_FLAG;
	if (nemojson_object_get_boolean(jobj, "rotate", 1) == 0)
		shell->pick.flags &= ~NEMOSHELL_PICK_ROTATE_FLAG;
	if (nemojson_object_get_boolean(jobj, "translate", 1) == 0)
		shell->pick.flags &= ~NEMOSHELL_PICK_TRANSLATE_FLAG;
	if (nemojson_object_get_boolean(jobj, "resize", 1) == 0)
		shell->pick.flags &= ~NEMOSHELL_PICK_RESIZE_FLAG;
}

static void nemoenvs_handle_nemoshell_pitch(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;

	shell->pitch.samples = nemojson_object_get_integer(jobj, "samples", 0);
	shell->pitch.max_duration = nemojson_object_get_integer(jobj, "maxDuration", 0);
	shell->pitch.friction = nemojson_object_get_double(jobj, "friction", 0.0f);
	shell->pitch.coefficient = nemojson_object_get_double(jobj, "coefficient", 0.0f);
}

int nemoenvs_handle_json_config(struct nemoenvs *envs, struct json_object *jobj)
{
	struct nemojson *json;
	struct json_object *tobj;
	struct json_object *cobj;
	int i;

	tobj = nemojson_object_get_object(jobj, "shells", NULL);
	if (tobj == NULL)
		return -1;

	cobj = nemojson_search_attribute(tobj,
			"id",
			nemojson_object_get_string(jobj, "useShellId", NULL));
	if (cobj == NULL)
		return -1;

	json = nemojson_create();
	nemojson_iterate_object(json, cobj);

	for (i = 0; i < nemojson_get_count(json); i++) {
		struct json_object *iobj = nemojson_get_object(json, i);
		const char *ikey = nemojson_get_key(json, i);

		if (strcmp(ikey, "screens") == 0) {
			nemoenvs_handle_nemoshell_screen(envs, iobj);
		} else if (strcmp(ikey, "inputs") == 0) {
			nemoenvs_handle_nemoshell_input(envs, iobj);
		} else if (strcmp(ikey, "evdevs") == 0) {
			nemoenvs_handle_nemoshell_evdev(envs, iobj);
		} else if (strcmp(ikey, "fullscreens") == 0) {
			nemoenvs_handle_nemoshell_fullscreen(envs, iobj);
		} else if (strcmp(ikey, "daemons") == 0) {
			nemoenvs_handle_nemoshell_daemon(envs, iobj);
		} else if (strcmp(ikey, "stages") == 0) {
			nemoenvs_handle_nemoshell_stage(envs, iobj);
		} else if (strcmp(ikey, "window") == 0) {
			nemoenvs_handle_nemoshell_window(envs, iobj);
		} else if (strcmp(ikey, "pick") == 0) {
			nemoenvs_handle_nemoshell_pick(envs, iobj);
		} else if (strcmp(ikey, "pitch") == 0) {
			nemoenvs_handle_nemoshell_pitch(envs, iobj);
		}
	}

	nemojson_destroy(json);

	return 0;
}
