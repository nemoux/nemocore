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
#include <nemotoken.h>
#include <nemoitem.h>
#include <nemomisc.h>
#include <nemolog.h>

int nemoshell_dispatch_message(void *data, const char *cmd, const char *path, struct itemone *one)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct nemocompz *compz = shell->compz;

	if (strcmp(cmd, "set") == 0) {
		struct itemattr *attr;
		const char *id;
		const char *name;
		const char *value;

		if (strcmp(path, "/nemoshell/backend") == 0) {
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
		} else if (strcmp(path, "/nemoshell/screen") == 0) {
			struct screenconfig *config;
			uint32_t nodeid = nemoitem_one_get_iattr(one, "nodeid", 0);
			uint32_t screenid = nemoitem_one_get_iattr(one, "screenid", 0);

			config = nemocompz_get_screen_config(compz, nodeid, screenid);
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
					} else if (strcmp(name, "refresh") == 0) {
						config->refresh = strtoul(value, NULL, 10);
					} else if (strcmp(name, "renderer") == 0) {
						config->renderer = strdup(value);
					} else if (strcmp(name, "transform") == 0) {
						config->transform = strdup(value);
					}
				}
			}
		} else if (strcmp(path, "/nemoshell/input") == 0) {
			struct inputconfig *config;
			const char *devnode = nemoitem_one_get_attr(one, "devnode");

			config = nemocompz_get_input_config(compz, devnode);
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
					} else if (strcmp(name, "nodeid") == 0) {
						config->nodeid = strtoul(value, NULL, 10);
					} else if (strcmp(name, "screenid") == 0) {
						config->screenid = strtoul(value, NULL, 10);
					} else if (strcmp(name, "transform") == 0) {
						config->transform = strdup(value);
					}
				}
			}
		} else if (strcmp(path, "/nemoshell/scene") == 0) {
			int32_t x = nemoitem_one_get_iattr(one, "x", 0);
			int32_t y = nemoitem_one_get_iattr(one, "y", 0);
			int32_t width = nemoitem_one_get_iattr(one, "width", 0);
			int32_t height = nemoitem_one_get_iattr(one, "height", 0);

			nemocompz_set_scene(compz, x, y, width, height);
		} else if (strcmp(path, "/nemoshell/scope") == 0) {
			int32_t x = nemoitem_one_get_iattr(one, "x", 0);
			int32_t y = nemoitem_one_get_iattr(one, "y", 0);
			int32_t width = nemoitem_one_get_iattr(one, "width", 0);
			int32_t height = nemoitem_one_get_iattr(one, "height", 0);

			nemocompz_set_scope(compz, x, y, width, height);
		} else if (strcmp(path, "/nemoshell/virtuio") == 0) {
			int32_t port = nemoitem_one_get_iattr(one, "port", 3333);
			int32_t fps = nemoitem_one_get_iattr(one, "fps", 60);
			int32_t x = nemoitem_one_get_iattr(one, "x", 0);
			int32_t y = nemoitem_one_get_iattr(one, "y", 0);
			int32_t width = nemoitem_one_get_iattr(one, "width", 0);
			int32_t height = nemoitem_one_get_iattr(one, "height", 0);

			virtuio_create(compz, port, fps, x, y, width, height);
		} else if (strcmp(path, "/nemoshell/tuio") == 0) {
			int32_t port = nemoitem_one_get_iattr(one, "port", 3333);
			int32_t max = nemoitem_one_get_iattr(one, "max", 16);
			const char *protocol = nemoitem_one_get_sattr(one, "protocol", "osc");

			tuio_create(compz,
					strcmp(protocol, "osc") == 0 ? NEMOTUIO_OSC_PROTOCOL : NEMOTUIO_XML_PROTOCOL,
					port, max);
		} else if (strcmp(path, "/nemoshell/plugin") == 0) {
			nemocompz_load_plugin(compz,
					nemoitem_one_get_attr(one, "path"),
					nemoitem_one_get_attr(one, "args"));
		} else if (strcmp(path, "/nemoshell/pick") == 0) {
			nemoitem_attr_for_each(attr, one) {
				name = nemoitem_attr_get_name(attr);
				value = nemoitem_attr_get_value(attr);

				if (strcmp(name, "samples") == 0) {
					shell->pick.samples = strtoul(value, NULL, 10);
				} else if (strcmp(name, "min_velocity") == 0) {
					shell->pick.min_velocity = strtod(value, NULL);
				} else if (strcmp(name, "rotate_degree") == 0) {
					shell->pick.rotate_degree = strtod(value, NULL);
				} else if (strcmp(name, "scale_degree") == 0) {
					shell->pick.scale_degree = strtod(value, NULL);
				} else if (strcmp(name, "rotate_distance") == 0) {
					shell->pick.rotate_distance = strtod(value, NULL);
				} else if (strcmp(name, "scale_distance") == 0) {
					shell->pick.scale_distance = strtod(value, NULL);
				} else if (strcmp(name, "fullscreen_scale") == 0) {
					shell->pick.fullscreen_scale = strtod(value, NULL);
				} else if (strcmp(name, "resize_interval") == 0) {
					shell->pick.resize_interval = strtod(value, NULL);
				}
			}
		} else if (strcmp(path, "/nemoshell/pitch") == 0) {
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
		} else if (strcmp(path, "/nemoshell/bin") == 0) {
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
		} else if (strcmp(path, "/nemoshell/fullscreen") == 0) {
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
	}

	return 0;
}
