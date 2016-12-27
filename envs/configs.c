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
#include <nemomisc.h>
#include <nemolog.h>

#include <nemoenvs.h>

static void nemoenvs_handle_set_nemoshell_screen(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	uint32_t nodeid = nemoitem_one_get_iattr(one, "nodeid", 0);
	uint32_t screenid = nemoitem_one_get_iattr(one, "screenid", 0);
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	screen = nemocompz_get_screen(compz, nodeid, screenid);
	if (screen != NULL) {
		int32_t width = screen->width;
		int32_t height = screen->height;
		int32_t x = 0, y = 0;
		float sx = 1.0f, sy = 1.0f;
		float r = 0.0f;
		float px = 0.0f, py = 0.0f;
		uint32_t refresh = 0;
		const char *transform = NULL;
		const char *scope = NULL;

		nemoitem_attr_for_each(attr, one) {
			name = nemoitem_attr_get_name(attr);
			value = nemoitem_attr_get_value(attr);

			if (strcmp(name, "x") == 0) {
				x = strtoul(value, NULL, 10);
			} else if (strcmp(name, "y") == 0) {
				y = strtoul(value, NULL, 10);
			} else if (strcmp(name, "width") == 0) {
				width = strtoul(value, NULL, 10);
			} else if (strcmp(name, "height") == 0) {
				height = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sx") == 0) {
				sx = strtod(value, NULL);
			} else if (strcmp(name, "sy") == 0) {
				sy = strtod(value, NULL);
			} else if (strcmp(name, "r") == 0) {
				r = strtod(value, NULL) * M_PI / 180.0f;
			} else if (strcmp(name, "px") == 0) {
				px = strtod(value, NULL);
			} else if (strcmp(name, "py") == 0) {
				py = strtod(value, NULL);
			} else if (strcmp(name, "refresh") == 0) {
				refresh = strtoul(value, NULL, 10);
			} else if (strcmp(name, "transform") == 0) {
				transform = value;
			} else if (strcmp(name, "scope") == 0) {
				scope = value;
			}
		}

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

static void nemoenvs_handle_set_nemoshell_input(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct inputnode *node;
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;
	const char *devnode = nemoitem_one_get_attr(one, "devnode");
	int32_t x = 0, y = 0;
	int32_t width = 0, height = 0;
	float sx = 1.0f, sy = 1.0f;
	float r = 0.0f;
	float px = 0.0f, py = 0.0f;
	uint32_t nodeid, screenid;
	uint32_t sampling = 0;
	float distance = 0.0f;
	const char *transform = NULL;
	int has_screen = 0;

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		if (strcmp(name, "x") == 0) {
			x = strtoul(value, NULL, 10);
		} else if (strcmp(name, "y") == 0) {
			y = strtoul(value, NULL, 10);
		} else if (strcmp(name, "width") == 0) {
			width = strtoul(value, NULL, 10);
		} else if (strcmp(name, "height") == 0) {
			height = strtoul(value, NULL, 10);
		} else if (strcmp(name, "sx") == 0) {
			sx = strtod(value, NULL);
		} else if (strcmp(name, "sy") == 0) {
			sy = strtod(value, NULL);
		} else if (strcmp(name, "r") == 0) {
			r = strtod(value, NULL) * M_PI / 180.0f;
		} else if (strcmp(name, "px") == 0) {
			px = strtod(value, NULL);
		} else if (strcmp(name, "py") == 0) {
			py = strtod(value, NULL);
		} else if (strcmp(name, "nodeid") == 0) {
			nodeid = strtoul(value, NULL, 10);
		} else if (strcmp(name, "screenid") == 0) {
			screenid = strtoul(value, NULL, 10);
			has_screen = 1;
		} else if (strcmp(name, "transform") == 0) {
			transform = value;
		} else if (strcmp(name, "sampling") == 0) {
			sampling = strtoul(value, NULL, 10);
		} else if (strcmp(name, "maximum_distance") == 0) {
			distance = strtod(value, NULL);
		}
	}

	wl_list_for_each(node, &compz->input_list, link) {
		if ((devnode != NULL && strcmp(node->devnode, devnode) == 0) ||
				(devnode == NULL && nemoinput_has_state(node, NEMOINPUT_CONFIG_STATE) == 0)) {
			if (has_screen != 0) {
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

static void nemoenvs_handle_set_nemoshell_evdev(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct inputnode *node;
	struct evdevnode *enode;
	struct itemattr *attr;
	const char *devpath = nemoitem_one_get_attr(one, "devpath");
	const char *id;
	const char *name;
	const char *value;
	int32_t x = 0, y = 0;
	int32_t width = 0, height = 0;
	float sx = 1.0f, sy = 1.0f;
	float r = 0.0f;
	float px = 0.0f, py = 0.0f;
	uint32_t nodeid, screenid;
	uint32_t sampling = 0;
	float distance = 0.0f;
	const char *transform = NULL;
	int has_screen = 0;

	enode = nemocompz_get_evdev(compz, devpath);
	if (enode == NULL) {
		int fd;

		fd = nemosession_open(compz->session, devpath, O_RDWR | O_NONBLOCK);
		if (fd < 0) {
			nemolog_error("ENVS", "failed to open evdev device '%s'\n", devpath);
			return;
		}

		enode = evdev_create_node(compz, devpath, fd);
		if (enode == NULL) {
			nemosession_close(compz->session, fd);
			return;
		}
	}

	node = &enode->base;

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		if (strcmp(name, "x") == 0) {
			x = strtoul(value, NULL, 10);
		} else if (strcmp(name, "y") == 0) {
			y = strtoul(value, NULL, 10);
		} else if (strcmp(name, "width") == 0) {
			width = strtoul(value, NULL, 10);
		} else if (strcmp(name, "height") == 0) {
			height = strtoul(value, NULL, 10);
		} else if (strcmp(name, "sx") == 0) {
			sx = strtod(value, NULL);
		} else if (strcmp(name, "sy") == 0) {
			sy = strtod(value, NULL);
		} else if (strcmp(name, "r") == 0) {
			r = strtod(value, NULL) * M_PI / 180.0f;
		} else if (strcmp(name, "px") == 0) {
			px = strtod(value, NULL);
		} else if (strcmp(name, "py") == 0) {
			py = strtod(value, NULL);
		} else if (strcmp(name, "nodeid") == 0) {
			nodeid = strtoul(value, NULL, 10);
		} else if (strcmp(name, "screenid") == 0) {
			screenid = strtoul(value, NULL, 10);
			has_screen = 1;
		} else if (strcmp(name, "transform") == 0) {
			transform = value;
		} else if (strcmp(name, "sampling") == 0) {
			sampling = strtoul(value, NULL, 10);
		} else if (strcmp(name, "maximum_distance") == 0) {
			distance = strtod(value, NULL);
		}
	}

	if (has_screen != 0) {
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

static void nemoenvs_handle_set_nemoshell_pick(struct nemoshell *shell, struct itemone *one)
{
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	shell->pick.flags = NEMOSHELL_PICK_ALL_FLAGS;

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		if (strcmp(name, "rotate_distance") == 0) {
			shell->pick.rotate_distance = strtod(value, NULL);
		} else if (strcmp(name, "scale_distance") == 0) {
			shell->pick.scale_distance = strtod(value, NULL);
		} else if (strcmp(name, "fullscreen_scale") == 0) {
			shell->pick.fullscreen_scale = strtod(value, NULL);
		} else if (strcmp(name, "resize_interval") == 0) {
			shell->pick.resize_interval = strtod(value, NULL);
		} else if (strcmp(name, "scale") == 0) {
			if (strcmp(value, "off") == 0)
				shell->pick.flags &= ~NEMOSHELL_PICK_SCALE_FLAG;
		} else if (strcmp(name, "rotate") == 0) {
			if (strcmp(value, "off") == 0)
				shell->pick.flags &= ~NEMOSHELL_PICK_ROTATE_FLAG;
		} else if (strcmp(name, "translate") == 0) {
			if (strcmp(value, "off") == 0)
				shell->pick.flags &= ~NEMOSHELL_PICK_TRANSLATE_FLAG;
		} else if (strcmp(name, "resize") == 0) {
			if (strcmp(value, "off") == 0)
				shell->pick.flags &= ~NEMOSHELL_PICK_RESIZE_FLAG;
		}
	}
}

static void nemoenvs_handle_set_nemoshell_pitch(struct nemoshell *shell, struct itemone *one)
{
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		if (strcmp(name, "samples") == 0) {
			shell->pitch.samples = strtoul(value, NULL, 10);
		} else if (strcmp(name, "max_duration") == 0) {
			shell->pitch.max_duration = strtoul(value, NULL, 10);
		} else if (strcmp(name, "friction") == 0) {
			shell->pitch.friction = strtod(value, NULL);
		} else if (strcmp(name, "coefficient") == 0) {
			shell->pitch.coefficient = strtod(value, NULL);
		}
	}
}

static void nemoenvs_handle_set_nemoshell_bin(struct nemoshell *shell, struct itemone *one)
{
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	nemoitem_attr_for_each(attr, one) {
		name = nemoitem_attr_get_name(attr);
		value = nemoitem_attr_get_value(attr);

		if (strcmp(name, "min_width") == 0) {
			shell->bin.min_width = strtoul(value, NULL, 10);
		} else if (strcmp(name, "min_height") == 0) {
			shell->bin.min_height = strtoul(value, NULL, 10);
		} else if (strcmp(name, "max_width") == 0) {
			shell->bin.max_width = strtoul(value, NULL, 10);
		} else if (strcmp(name, "max_height") == 0) {
			shell->bin.max_height = strtoul(value, NULL, 10);
		}

		if (shell->bin.max_width > 0 && shell->bin.max_height > 0)
			nemocompz_set_output(shell->compz, 0, 0, shell->bin.max_width, shell->bin.max_height);
	}
}

static void nemoenvs_handle_set_nemoshell_fullscreen(struct nemoshell *shell, struct itemone *one)
{
	struct shellscreen *screen;
	struct itemattr *attr;
	const char *path = nemoitem_one_get_path(one);
	const char *name;
	const char *value;

	screen = nemoshell_get_fullscreen(shell, path);
	if (screen != NULL) {
		nemoitem_attr_for_each(attr, one) {
			name = nemoitem_attr_get_name(attr);
			value = nemoitem_attr_get_value(attr);

			if (strcmp(name, "sx") == 0) {
				screen->sx = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sy") == 0) {
				screen->sy = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sw") == 0) {
				screen->sw = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sh") == 0) {
				screen->sh = strtoul(value, NULL, 10);
			} else if (strcmp(name, "dx") == 0) {
				screen->dx = strtoul(value, NULL, 10);
			} else if (strcmp(name, "dy") == 0) {
				screen->dy = strtoul(value, NULL, 10);
			} else if (strcmp(name, "dw") == 0) {
				screen->dw = strtoul(value, NULL, 10);
			} else if (strcmp(name, "dh") == 0) {
				screen->dh = strtoul(value, NULL, 10);
				screen->has_screen = 0;
			} else if (strcmp(name, "dr") == 0) {
				screen->dr = strtoul(value, NULL, 10);
			} else if (strcmp(name, "nodeid") == 0) {
				screen->nodeid = strtoul(value, NULL, 10);
			} else if (strcmp(name, "screenid") == 0) {
				screen->screenid = strtoul(value, NULL, 10);
				screen->has_screen = 1;
			} else if (strcmp(name, "type") == 0) {
				if (strcmp(value, "pick") == 0)
					screen->type = NEMOSHELL_FULLSCREEN_PICK_TYPE;
				else if (strcmp(value, "pitch") == 0)
					screen->type = NEMOSHELL_FULLSCREEN_PITCH_TYPE;
				else
					screen->type = NEMOSHELL_FULLSCREEN_NORMAL_TYPE;
			} else if (strcmp(name, "focus") == 0) {
				if (strcmp(value, "all") == 0)
					screen->focus = NEMOSHELL_FULLSCREEN_ALL_FOCUS;
				else
					screen->focus = NEMOSHELL_FULLSCREEN_NONE_FOCUS;
			} else if (strcmp(name, "fixed") == 0) {
				if (strcmp(value, "on") == 0)
					screen->fixed = 1;
				else
					screen->fixed = 0;
			} else if (strcmp(name, "target") == 0) {
				screen->target = strtoul(value, NULL, 10);
			}
		}
	}
}

static void nemoenvs_handle_set_nemoshell_stage(struct nemoshell *shell, struct itemone *one)
{
	struct shellstage *stage;
	struct itemattr *attr;
	const char *path = nemoitem_one_get_path(one);
	const char *name;
	const char *value;

	stage = nemoshell_get_stage(shell, path);
	if (stage != NULL) {
		nemoitem_attr_for_each(attr, one) {
			name = nemoitem_attr_get_name(attr);
			value = nemoitem_attr_get_value(attr);

			if (strcmp(name, "sx") == 0) {
				stage->sx = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sy") == 0) {
				stage->sy = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sw") == 0) {
				stage->sw = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sh") == 0) {
				stage->sh = strtoul(value, NULL, 10);
			} else if (strcmp(name, "dr") == 0) {
				stage->dr = strtoul(value, NULL, 10);
			}
		}
	}
}

static void nemoenvs_handle_set_nemoshell_show(struct nemoshell *shell, struct itemone *one)
{
	char contents[128];
	int threads;
	int tilesize;
	int duration;
	int distance;

	threads = nemoitem_one_get_iattr(one, "threads", 0);
	if (threads > 0) {
		snprintf(contents, sizeof(contents), "%d", threads);

		setenv("NEMOSHOW_THREADS", contents, 1);
	} else {
		unsetenv("NEMOSHOW_THREADS");
	}

	tilesize = nemoitem_one_get_iattr(one, "tilesize", 0);
	if (tilesize > 0) {
		snprintf(contents, sizeof(contents), "%d", tilesize);

		setenv("NEMOSHOW_TILESIZE", contents, 1);
	} else {
		unsetenv("NEMOSHOW_TILESIZE");
	}

	duration = nemoitem_one_get_iattr(one, "single_click_duration", 0);
	if (duration > 0) {
		snprintf(contents, sizeof(contents), "%d", duration);

		setenv("NEMOSHOW_SINGLE_CLICK_DURATION", contents, 1);
	} else {
		unsetenv("NEMOSHOW_SINGLE_CLICK_DURATION");
	}

	distance = nemoitem_one_get_iattr(one, "single_click_distance", 0);
	if (distance > 0) {
		snprintf(contents, sizeof(contents), "%d", distance);

		setenv("NEMOSHOW_SINGLE_CLICK_DISTANCE", contents, 1);
	} else {
		unsetenv("NEMOSHOW_SINGLE_CLICK_DISTANCE");
	}
}

int nemoenvs_set_config(struct nemoenvs *envs, struct itemone *one)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;
	const char *path = nemoitem_one_get_path(one);

	if (namespace_has_prefix(path, "/nemoshell/screen") != 0) {
		nemoenvs_handle_set_nemoshell_screen(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/input") != 0) {
		nemoenvs_handle_set_nemoshell_input(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/evdev") != 0) {
		nemoenvs_handle_set_nemoshell_evdev(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/scene") != 0) {
		int32_t x = nemoitem_one_get_iattr(one, "x", 0);
		int32_t y = nemoitem_one_get_iattr(one, "y", 0);
		int32_t width = nemoitem_one_get_iattr(one, "width", 0);
		int32_t height = nemoitem_one_get_iattr(one, "height", 0);

		nemocompz_set_scene(compz, x, y, width, height);
	} else if (namespace_has_prefix(path, "/nemoshell/scope") != 0) {
		int32_t x = nemoitem_one_get_iattr(one, "x", 0);
		int32_t y = nemoitem_one_get_iattr(one, "y", 0);
		int32_t width = nemoitem_one_get_iattr(one, "width", 0);
		int32_t height = nemoitem_one_get_iattr(one, "height", 0);

		nemocompz_set_scope(compz, x, y, width, height);
	} else if (namespace_has_prefix(path, "/nemoshell/virtuio") != 0) {
		int32_t port = nemoitem_one_get_iattr(one, "port", 3333);
		int32_t fps = nemoitem_one_get_iattr(one, "fps", 60);
		int32_t x = nemoitem_one_get_iattr(one, "x", 0);
		int32_t y = nemoitem_one_get_iattr(one, "y", 0);
		int32_t width = nemoitem_one_get_iattr(one, "width", 0);
		int32_t height = nemoitem_one_get_iattr(one, "height", 0);

		virtuio_create(compz, port, fps, x, y, width, height);
	} else if (namespace_has_prefix(path, "/nemoshell/tuio") != 0) {
		int32_t port = nemoitem_one_get_iattr(one, "port", 3333);
		int32_t max = nemoitem_one_get_iattr(one, "max", 16);
		const char *protocol = nemoitem_one_get_sattr(one, "protocol", "osc");

		tuio_create(compz,
				namespace_has_prefix(protocol, "osc") == 0 ? NEMOTUIO_OSC_PROTOCOL : NEMOTUIO_XML_PROTOCOL,
				port, max);
	} else if (namespace_has_prefix(path, "/nemoshell/plugin") != 0) {
		nemocompz_load_plugin(compz,
				nemoitem_one_get_attr(one, "path"),
				nemoitem_one_get_attr(one, "args"));
	} else if (namespace_has_prefix(path, "/nemoshell/pick") != 0) {
		nemoenvs_handle_set_nemoshell_pick(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/pitch") != 0) {
		nemoenvs_handle_set_nemoshell_pitch(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/bin") != 0) {
		nemoenvs_handle_set_nemoshell_bin(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/fullscreen") != 0) {
		nemoenvs_handle_set_nemoshell_fullscreen(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/stage") != 0) {
		nemoenvs_handle_set_nemoshell_stage(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/show") != 0) {
		nemoenvs_handle_set_nemoshell_show(shell, one);
	} else if (namespace_has_prefix(path, "/nemoshell/idle") != 0) {
		uint32_t timeout;

		timeout = nemoitem_one_get_iattr(one, "timeout", 0);

		nemocompz_set_idle_timeout(shell->compz, timeout);
	} else if (namespace_has_prefix(path, "/nemoshell/legacy") != 0) {
		envs->legacy.pick_taps = nemoitem_one_get_iattr(one, "pick_taps", 3);
	} else if (namespace_has_prefix(path, "/nemoshell/terminal") != 0) {
		nemoenvs_set_terminal_path(envs, nemoitem_one_get_attr(one, "path"));
		nemoenvs_set_terminal_args(envs, nemoitem_one_get_attr(one, "args"));
	} else if (namespace_has_prefix(path, "/nemoshell/xserver") != 0) {
		nemoenvs_set_xserver_path(envs, nemoitem_one_get_attr(one, "path"));
		nemoenvs_set_xserver_node(envs, nemoitem_one_get_attr(one, "node"));
	} else if (namespace_has_prefix(path, "/nemoshell/background") != 0) {
		nemoenvs_set_service(envs, one);
	} else if (namespace_has_prefix(path, "/nemoshell/daemon") != 0) {
		nemoenvs_set_service(envs, one);
	} else if (namespace_has_prefix(path, "/nemoshell/screensaver") != 0) {
		nemoenvs_set_service(envs, one);
	}

	return 0;
}

void nemoenvs_put_config(struct nemoenvs *envs, const char *path)
{
	struct nemoshell *shell = envs->shell;
	struct nemocompz *compz = shell->compz;

	if (namespace_has_prefix(path, "/nemoshell/fullscreen") != 0) {
		nemoshell_put_fullscreen(shell, path);
	} else if (namespace_has_prefix(path, "/nemoshell/stage") != 0) {
		nemoshell_put_stage(shell, path);
	}
}
