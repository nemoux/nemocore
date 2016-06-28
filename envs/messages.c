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
#include <wayland-nemo-shell-server-protocol.h>

#include <shell.h>
#include <compz.h>
#include <canvas.h>
#include <subcanvas.h>
#include <view.h>
#include <seat.h>
#include <keyboard.h>
#include <pointer.h>
#include <touch.h>
#include <stick.h>
#include <keypad.h>
#include <datadevice.h>
#include <screen.h>
#include <plugin.h>
#include <viewanimation.h>
#include <busycursor.h>
#include <vieweffect.h>
#include <drmbackend.h>
#include <fbbackend.h>
#include <evdevbackend.h>
#include <tuio.h>
#include <virtuio.h>
#include <waylandshell.h>
#include <xdgshell.h>
#include <nemoshell.h>
#include <prochelper.h>
#include <namespacehelper.h>
#include <nemotoken.h>
#include <nemoitem.h>
#include <nemomisc.h>
#include <nemolog.h>

#include <nemoenvs.h>

static void nemoenvs_handle_set_nemoshell_backend(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	const char *name;

	name = nemoitem_one_get_attr(one, "name");
	if (name != NULL) {
		if (strcmp(name, "drm") == 0) {
			drmbackend_create(compz, nemoitem_one_get_attr(one, "rendernode"));
		} else if (strcmp(name, "fb") == 0) {
			fbbackend_create(compz, nemoitem_one_get_attr(one, "rendernode"));
		} else if (strcmp(name, "evdev") == 0) {
			evdevbackend_create(compz);
		}
	}
}

static void nemoenvs_handle_set_nemoshell_screen(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct screenconfig *config;
	uint32_t nodeid = nemoitem_one_get_iattr(one, "nodeid", 0);
	uint32_t screenid = nemoitem_one_get_iattr(one, "screenid", 0);
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	config = nemocompz_get_screen_config(compz, nodeid, screenid);
	if (config == NULL)
		config = nemocompz_set_screen_config(compz, nodeid, screenid);
	if (config != NULL) {
		nemoitem_attr_for_each(attr, one) {
			name = nemoitem_attr_get_name(attr);
			value = nemoitem_attr_get_value(attr);

			if (strcmp(name, "x") == 0) {
				config->x = strtoul(value, NULL, 10);
			} else if (strcmp(name, "y") == 0) {
				config->y = strtoul(value, NULL, 10);
			} else if (strcmp(name, "width") == 0) {
				config->width = strtoul(value, NULL, 10);
			} else if (strcmp(name, "height") == 0) {
				config->height = strtoul(value, NULL, 10);
			} else if (strcmp(name, "sx") == 0) {
				config->sx = strtod(value, NULL);
			} else if (strcmp(name, "sy") == 0) {
				config->sy = strtod(value, NULL);
			} else if (strcmp(name, "r") == 0) {
				config->r = strtod(value, NULL) * M_PI / 180.0f;
			} else if (strcmp(name, "px") == 0) {
				config->px = strtod(value, NULL);
			} else if (strcmp(name, "py") == 0) {
				config->py = strtod(value, NULL);
			} else if (strcmp(name, "refresh") == 0) {
				config->refresh = strtoul(value, NULL, 10);
			} else if (strcmp(name, "renderer") == 0) {
				config->renderer = strdup(value);
			} else if (strcmp(name, "transform") == 0) {
				config->transform = strdup(value);
			}
		}

		screen = nemocompz_get_screen(compz, nodeid, screenid);
		if (screen != NULL) {
			if (screen->width != config->width || screen->height != config->height)
				nemoscreen_switch_mode(screen, config->width, config->height, config->refresh);

			if (config->transform != NULL) {
				nemoscreen_set_transform(screen, config->transform);
			} else {
				if (screen->x != config->x || screen->y != config->y)
					nemoscreen_set_position(screen, config->x, config->y);
				if (screen->geometry.sx != config->sx || screen->geometry.sy != config->sy)
					nemoscreen_set_scale(screen, config->sx, config->sy);
				if (screen->geometry.r != config->r)
					nemoscreen_set_rotation(screen, config->r);
				if (screen->geometry.px != config->py)
					nemoscreen_set_pivot(screen, config->px, config->py);
			}

			nemoscreen_schedule_repaint(screen);
		}
	}
}

static void nemoenvs_handle_set_nemoshell_input(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct inputnode *node;
	struct inputconfig *config;
	const char *devnode = nemoitem_one_get_attr(one, "devnode");
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	config = nemocompz_get_input_config(compz, devnode);
	if (config == NULL)
		config = nemocompz_set_input_config(compz, devnode);
	if (config != NULL) {
		nemoitem_attr_for_each(attr, one) {
			name = nemoitem_attr_get_name(attr);
			value = nemoitem_attr_get_value(attr);

			if (strcmp(name, "x") == 0) {
				config->x = strtoul(value, NULL, 10);
			} else if (strcmp(name, "y") == 0) {
				config->y = strtoul(value, NULL, 10);
			} else if (strcmp(name, "width") == 0) {
				config->width = strtoul(value, NULL, 10);
			} else if (strcmp(name, "height") == 0) {
				config->height = strtoul(value, NULL, 10);
				config->has_screen = 0;
			} else if (strcmp(name, "nodeid") == 0) {
				config->nodeid = strtoul(value, NULL, 10);
			} else if (strcmp(name, "screenid") == 0) {
				config->screenid = strtoul(value, NULL, 10);
				config->has_screen = 1;
			} else if (strcmp(name, "transform") == 0) {
				config->transform = strdup(value);
			} else if (strcmp(name, "sampling") == 0) {
				config->sampling = strtoul(value, NULL, 10);
			}
		}

		node = nemocompz_get_input(compz, devnode);
		if (node != NULL) {
			if (config->has_screen != 0) {
				screen = nemocompz_get_screen(compz, config->nodeid, config->screenid);
				if (node->screen != screen)
					nemoinput_set_screen(node, screen);
			} else {
				nemoinput_set_geometry(node,
						config->x,
						config->y,
						config->width,
						config->height);

				if (config->transform != NULL)
					nemoinput_set_transform(node, config->transform);

				nemoinput_set_sampling(node, config->sampling);
			}
		}
	}
}

static void nemoenvs_handle_set_nemoshell_scene(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	int32_t x = nemoitem_one_get_iattr(one, "x", 0);
	int32_t y = nemoitem_one_get_iattr(one, "y", 0);
	int32_t width = nemoitem_one_get_iattr(one, "width", 0);
	int32_t height = nemoitem_one_get_iattr(one, "height", 0);

	nemocompz_set_scene(compz, x, y, width, height);
}

static void nemoenvs_handle_set_nemoshell_scope(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	int32_t x = nemoitem_one_get_iattr(one, "x", 0);
	int32_t y = nemoitem_one_get_iattr(one, "y", 0);
	int32_t width = nemoitem_one_get_iattr(one, "width", 0);
	int32_t height = nemoitem_one_get_iattr(one, "height", 0);

	nemocompz_set_scope(compz, x, y, width, height);
}

static void nemoenvs_handle_set_nemoshell_virtuio(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	int32_t port = nemoitem_one_get_iattr(one, "port", 3333);
	int32_t fps = nemoitem_one_get_iattr(one, "fps", 60);
	int32_t x = nemoitem_one_get_iattr(one, "x", 0);
	int32_t y = nemoitem_one_get_iattr(one, "y", 0);
	int32_t width = nemoitem_one_get_iattr(one, "width", 0);
	int32_t height = nemoitem_one_get_iattr(one, "height", 0);

	virtuio_create(compz, port, fps, x, y, width, height);
}

static void nemoenvs_handle_set_nemoshell_tuio(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	int32_t port = nemoitem_one_get_iattr(one, "port", 3333);
	int32_t max = nemoitem_one_get_iattr(one, "max", 16);
	const char *protocol = nemoitem_one_get_sattr(one, "protocol", "osc");

	tuio_create(compz,
			strcmp(protocol, "osc") == 0 ? NEMOTUIO_OSC_PROTOCOL : NEMOTUIO_XML_PROTOCOL,
			port, max);
}

static void nemoenvs_handle_set_nemoshell_plugin(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;

	nemocompz_load_plugin(compz,
			nemoitem_one_get_attr(one, "path"),
			nemoitem_one_get_attr(one, "args"));
}

static void nemoenvs_handle_set_nemoshell_pick(struct nemoshell *shell, struct itemone *one)
{
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

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
	}
}

static void nemoenvs_handle_set_nemoshell_fullscreen(struct nemoshell *shell, struct itemone *one)
{
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	id = nemoitem_one_get_attr(one, "id");
	if (id != NULL) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(shell, id);
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
				} else if (strcmp(name, "dr") == 0) {
					screen->dr = strtoul(value, NULL, 10);
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
				}
			}
		}
	}
}

static void nemoenvs_handle_set_nemoshell_font(struct nemoshell *shell, struct itemone *one)
{
	char contents[1024];

	nemoitem_one_save(one, contents, ' ');
	setenv("NEMOSHELL_FONT", contents, 1);
}

int nemoenvs_dispatch_system_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)data;

	if (strcmp(dst, "/nemoshell") == 0) {
		if (strcmp(cmd, "set") == 0) {
			if (strcmp(path, "/nemoshell/backend") == 0) {
				nemoenvs_handle_set_nemoshell_backend(shell, one);
			} else if (strcmp(path, "/nemoshell/screen") == 0) {
				nemoenvs_handle_set_nemoshell_screen(shell, one);
			} else if (strcmp(path, "/nemoshell/input") == 0) {
				nemoenvs_handle_set_nemoshell_input(shell, one);
			} else if (strcmp(path, "/nemoshell/scene") == 0) {
				nemoenvs_handle_set_nemoshell_scene(shell, one);
			} else if (strcmp(path, "/nemoshell/scope") == 0) {
				nemoenvs_handle_set_nemoshell_scope(shell, one);
			} else if (strcmp(path, "/nemoshell/virtuio") == 0) {
				nemoenvs_handle_set_nemoshell_virtuio(shell, one);
			} else if (strcmp(path, "/nemoshell/tuio") == 0) {
				nemoenvs_handle_set_nemoshell_tuio(shell, one);
			} else if (strcmp(path, "/nemoshell/plugin") == 0) {
				nemoenvs_handle_set_nemoshell_plugin(shell, one);
			} else if (strcmp(path, "/nemoshell/pick") == 0) {
				nemoenvs_handle_set_nemoshell_pick(shell, one);
			} else if (strcmp(path, "/nemoshell/pitch") == 0) {
				nemoenvs_handle_set_nemoshell_pitch(shell, one);
			} else if (strcmp(path, "/nemoshell/bin") == 0) {
				nemoenvs_handle_set_nemoshell_bin(shell, one);
			} else if (strcmp(path, "/nemoshell/fullscreen") == 0) {
				nemoenvs_handle_set_nemoshell_fullscreen(shell, one);
			} else if (strcmp(path, "/nemoshell/font") == 0) {
				nemoenvs_handle_set_nemoshell_font(shell, one);
			}
		}
	}

	return 0;
}

int nemoenvs_dispatch_device_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	if (strcmp(dst, "/nemoshell") == 0) {
	}

	return 0;
}

int nemoenvs_dispatch_config_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	if (namespace_has_prefix(path, "/nemoshell") != 0) {
		if (strcmp(cmd, "set") == 0) {
			struct itemone *tone;
			const char *id;

			id = nemoitem_one_get_attr(one, "id");
			if (id != NULL) {
				tone = nemoitem_search_attr(envs->configs, path, "id", id);
				if (tone != NULL)
					nemoitem_one_copy_attr(tone, one);
				else
					nemoitem_attach_one(envs->configs,
							nemoitem_one_clone(one));
			} else {
				nemoitem_attach_one(envs->configs,
						nemoitem_one_clone(one));
			}
		} else if (strcmp(cmd, "get") == 0) {
			struct itemone *tone;
			const char *id;

			id = nemoitem_one_get_attr(one, "id");
			if (id != NULL) {
				tone = nemoitem_search_attr(envs->configs, path, "id", id);
				if (tone != NULL) {
					char contents[1024] = { 0 };

					nemoitem_one_save(tone, contents, ' ');

					nemoenvs_reply(envs, "%s %s set %s", dst, src, contents);
				}
			} else {
				nemoitem_for_each(tone, envs->configs) {
					if (nemoitem_one_has_path(tone, path) != 0) {
						char contents[1024] = { 0 };

						nemoitem_one_save(tone, contents, ' ');

						nemoenvs_reply(envs, "%s %s set %s", dst, src, contents);
					}
				}
			}
		} else if (strcmp(cmd, "put") == 0) {
			struct itemone *tone;
			const char *id;

			id = nemoitem_one_get_attr(one, "id");
			if (id != NULL) {
				tone = nemoitem_search_attr(envs->configs, path, "id", id);
				if (tone != NULL)
					nemoitem_one_destroy(tone);
			}
		}
	}

	return 0;
}

int nemoenvs_dispatch_client_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	if (strcmp(dst, "/nemoshell") == 0) {
		if (strcmp(cmd, "rep") == 0) {
			if (strcmp(path, "/check/live") == 0) {
				nemomsg_set_client(envs->msg, src,
						nemomsg_get_source_ip(envs->msg),
						nemomsg_get_source_port(envs->msg));
			}
		}
	} else {
		nemomsg_send_message(envs->msg, dst,
				nemomsg_get_source_buffer(envs->msg),
				nemomsg_get_source_size(envs->msg));
	}

	return 0;
}
