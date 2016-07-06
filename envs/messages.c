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
				transform = strdup(value);
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

		nemoscreen_schedule_repaint(screen);
	}
}

static void nemoenvs_handle_set_nemoshell_input(struct nemoshell *shell, struct itemone *one)
{
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct inputnode *node;
	const char *devnode = nemoitem_one_get_attr(one, "devnode");
	struct itemattr *attr;
	const char *id;
	const char *name;
	const char *value;

	node = nemocompz_get_input(compz, devnode);
	if (node != NULL) {
		int32_t x = 0, y = 0;
		int32_t width = 0, height = 0;
		float sx = 1.0f, sy = 1.0f;
		float r = 0.0f;
		float px = 0.0f, py = 0.0f;
		uint32_t nodeid, screenid;
		uint32_t sampling = 0;
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
			}
		}

		if (has_screen != 0) {
			screen = nemocompz_get_screen(compz, nodeid, screenid);
			if (node->screen != screen)
				nemoinput_set_screen(node, screen);
		} else {
			nemoinput_put_screen(node);

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

	nemoitem_one_save(one, contents, ' ', '\"');
	setenv("NEMOSHELL_FONT", contents, 1);
}

int nemoenvs_dispatch_system_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)data;

	if (strcmp(dst, "/nemoshell") == 0) {
		if (strcmp(cmd, "set") == 0) {
			if (strcmp(path, "/nemoshell/screen") == 0) {
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
	struct nemoshell *shell = (struct nemoshell *)data;
	struct nemocompz *compz = shell->compz;

	if (strcmp(dst, "/nemoshell") == 0) {
		if (strcmp(cmd, "dev") == 0) {
			if (strcmp(path, "/nemoshell/screen") == 0) {
				struct nemoscreen *screen;
				struct itemone *tone;
				const char *id;

				wl_list_for_each(screen, &compz->screen_list, link) {
					tone = nemoitem_search_format(envs->configs, path, ' ', "nodeid %d screenid %d", screen->node->nodeid, screen->screenid);
					id = tone != NULL ? nemoitem_one_get_attr(tone, "id") : NULL;

					if (id != NULL)
						nemoenvs_reply(envs, "%s %s dev %s nodeid \"%d\" screenid \"%d\" id \"%s\"", dst, src, path, screen->node->nodeid, screen->screenid, id);
					else
						nemoenvs_reply(envs, "%s %s dev %s nodeid \"%d\" screenid \"%d\"", dst, src, path, screen->node->nodeid, screen->screenid);
				}
			} else if (strcmp(path, "/nemoshell/screen/mode") == 0) {
				struct nemoscreen *screen;
				struct nemomode *mode;
				uint32_t nodeid = nemoitem_one_get_iattr(one, "nodeid", 0);
				uint32_t screenid = nemoitem_one_get_iattr(one, "screenid", 0);

				screen = nemocompz_get_screen(compz, nodeid, screenid);
				if (screen != NULL) {
					wl_list_for_each(mode, &screen->mode_list, link) {
						nemoenvs_reply(envs, "%s %s dev %s nodeid \"%d\" screenid \"%d\" width \"%d\" height \"%d\"", dst, src, path, screen->node->nodeid, screen->screenid, mode->width, mode->height);
					}
				}
			} else if (strcmp(path, "/nemoshell/input") == 0) {
				struct inputnode *node;
				struct itemone *tone;
				const char *id;

				wl_list_for_each(node, &compz->input_list, link) {
					tone = nemoitem_search_attr(envs->configs, path, "devnode", node->devnode);
					id = tone != NULL ? nemoitem_one_get_attr(tone, "id") : NULL;

					if (id != NULL)
						nemoenvs_reply(envs, "%s %s dev %s devnode \"%s\" id \"%s\"", dst, src, path, node->devnode, id);
					else
						nemoenvs_reply(envs, "%s %s dev %s devnode \"%s\"", dst, src, path, node->devnode);
				}
			} else if (strcmp(path, "/nemoshell/input/touch") == 0) {
				struct inputnode *node;
				struct itemone *tone;
				const char *id;

				wl_list_for_each(node, &compz->input_list, link) {
					if (node->type & NEMOINPUT_TOUCH_TYPE) {
						tone = nemoitem_search_attr(envs->configs, path, "devnode", node->devnode);
						id = tone != NULL ? nemoitem_one_get_attr(tone, "id") : NULL;

						if (id != NULL)
							nemoenvs_reply(envs, "%s %s dev %s devnode \"%s\" id \"%s\"", dst, src, path, node->devnode, id);
						else
							nemoenvs_reply(envs, "%s %s dev %s devnode \"%s\"", dst, src, path, node->devnode);
					}
				}
			}
		}
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
					nemoitem_one_destroy(tone);

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

					nemoitem_one_save(tone, contents, ' ', '\"');

					nemoenvs_reply(envs, "%s %s set %s", dst, src, contents);
				}
			} else {
				nemoitem_for_each(tone, envs->configs) {
					if (nemoitem_one_has_path(tone, path) != 0) {
						char contents[1024] = { 0 };

						nemoitem_one_save(tone, contents, ' ', '\"');

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
