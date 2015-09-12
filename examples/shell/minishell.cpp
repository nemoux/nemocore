#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <linux/input.h>
#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <canvas.h>
#include <subcanvas.h>
#include <actor.h>
#include <view.h>
#include <viewanimation.h>
#include <screen.h>
#include <seat.h>
#include <keyboard.h>
#include <pointer.h>
#include <touch.h>
#include <virtuio.h>
#include <session.h>
#include <binding.h>
#include <plugin.h>
#include <datadevice.h>
#include <drmbackend.h>
#include <fbbackend.h>
#include <evdevbackend.h>
#include <tuiobackend.h>
#include <waylandhelper.h>
#include <talehelper.h>
#include <showhelper.h>
#include <nemoxml.h>
#include <nemolog.h>
#include <nemoitem.h>
#include <nemomisc.h>

#include <minishell.h>
#include <minigrab.h>
#include <miniyoyo.hpp>
#include <minipalm.h>
#include <skiahelper.hpp>

#include <showitem.h>
#include <showitem.hpp>

#ifdef NEMOUX_WITH_XWAYLAND
#include <xserver.h>
#endif

static void minishell_load_configs(struct nemoshell *shell, const char *configpath)
{
	struct nemocompz *compz = shell->compz;
	struct nemoxml *xml;
	struct xmlnode *node;
	int i, j;

	xml = nemoxml_create();
	nemoxml_load_file(xml, configpath);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->name, "screen") == 0 ||
				strcmp(node->name, "input") == 0 ||
				strcmp(node->name, "plugin") == 0) {
			i = nemoitem_set(compz->configs, node->path);

			for (j = 0; j < node->nattrs; j++) {
				nemoitem_set_attr(compz->configs, i,
						node->attrs[j*2+0],
						node->attrs[j*2+1]);
			}
		}
	}

	nemolist_for_each(node, &xml->nodes, nodelink) {
		i = nemoitem_set(shell->configs, node->path);

		for (j = 0; j < node->nattrs; j++) {
			nemoitem_set_attr(shell->configs, i,
					node->attrs[j*2+0],
					node->attrs[j*2+1]);
		}
	}

	nemoxml_destroy(xml);
}

static void minishell_load_virtuio(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;
	int index, i;
	int port, fps;
	int x, y, width, height;

	for (index = 0;
			(index = nemoitem_get(shell->configs, "//nemoshell/virtuio", index)) >= 0;
			index++) {
		port = nemoitem_get_iattr(shell->configs, index, "port");
		fps = nemoitem_get_iattr(shell->configs, index, "fps");
		x = nemoitem_get_iattr(shell->configs, index, "x");
		y = nemoitem_get_iattr(shell->configs, index, "y");
		width = nemoitem_get_iattr(shell->configs, index, "width");
		height = nemoitem_get_iattr(shell->configs, index, "height");

		virtuio_create(compz, port, fps, x, y, width, height);
	}
}

static void minishell_launch_background(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;
	int index, i;

	index = nemoitem_get(shell->configs, "//nemoshell/background", 0);
	if (index >= 0) {
		char *argv[16];
		int32_t width, height;

		width = nemocompz_get_scene_width(compz);
		height = nemocompz_get_scene_height(compz);

		argv[0] = nemoitem_get_attr(shell->configs, index, "path");
		argv[1] = strdup("-w");
		asprintf(&argv[2], "%d", width);
		argv[3] = strdup("-h");
		asprintf(&argv[4], "%d", height);
		argv[5] = strdup("-b");

		for (i = 6;
				(argv[i] = nemoitem_get_vattr(shell->configs, index, "arg%d", i - 6)) != NULL;
				i++);
		argv[i] = NULL;

		wayland_execute_client(compz->display, argv[0], argv, NULL, NULL);

		free(argv[1]);
		free(argv[2]);
		free(argv[3]);
		free(argv[4]);
		free(argv[5]);
	}
}

static void minishell_launch_apps(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;
	struct wl_client *client;
	char *argv[16], *envp[16];
	int index, i;

	for (index = 0;
			(index = nemoitem_get(shell->configs, "//nemoshell/start/item", index)) >= 0;
			index++) {
		argv[0] = nemoitem_get_attr(shell->configs, index, "path");
		for (i = 1;
				(argv[i] = nemoitem_get_vattr(shell->configs, index, "arg%d", i - 1)) != NULL;
				i++);
		argv[i] = NULL;

		for (i = 0;
				(envp[i] = nemoitem_get_vattr(shell->configs, index, "env%d", i)) != NULL;
				i++);
		envp[i] = NULL;

		client = wayland_execute_client(compz->display, argv[0], argv, envp, NULL);
		if (client != NULL) {
			struct nemoscreen *screen;
			struct clientstate *state;

			screen = nemocompz_get_main_screen(compz);
			state = nemoshell_create_client_state(client);
			if (state != NULL && screen != NULL) {
				state->x = screen->width / 2;
				state->y = 0;
				state->dx = 0.0f;
				state->dy = 0.0f;
				state->width = screen->width / 2;
				state->height = screen->height;
				state->is_maximized = 1;
				state->flags = 0x0;
			}
		}
	}
}

static void minishell_handle_escape_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		nemolog_message("SHELL", "exit nemoshell by escape key\n");

		nemocompz_destroy_clients(compz);
		nemocompz_exit(compz);
	}
}

static void minishell_handle_left_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data)
{
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (pointer->focus != NULL && pointer->focus->canvas != NULL) {
			struct nemocanvas *parent = nemosubcanvas_get_main_canvas(pointer->focus->canvas);
			struct shellbin *bin = nemoshell_get_bin(pointer->focus->canvas);

			nemoview_update_layer(pointer->focus);

			if (bin != NULL) {
				nemopointer_set_keyboard_focus(pointer, pointer->focus);
				datadevice_set_focus(pointer->seat, pointer->focus);
			}
		} else if (pointer->focus != NULL && pointer->focus->actor != NULL) {
			nemoview_update_layer(pointer->focus);
			nemopointer_set_keyboard_focus(pointer, pointer->focus);
		}

		wl_signal_emit(&compz->activate_signal, pointer->focus);
	}
}

static void minishell_handle_key_binding(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		struct nemopointer *pointer = (struct nemopointer *)data;

		nemopointer_set_keyboard(pointer, keyboard);

		nemolog_message("SHELL", "bind keyboard(%s) to pointer(%s)\n", keyboard->node->devnode, pointer->node->devnode);
	}
}

static void minishell_handle_touch_binding(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)data;
	struct nemoscreen *screen;

	screen = nemocompz_get_screen_on(compz, pointer->x, pointer->y);

	nemoinput_set_screen(tp->touch->node, screen);

	nemolog_message("SHELL", "bind touch(%s) to screen(%d:%d)\n", tp->touch->node->devnode, screen->node->nodeid, screen->screenid);
}

static void minishell_handle_right_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data)
{
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		pointer->bindings[0] = nemocompz_add_key_binding(compz, KEY_ENTER, 0, minishell_handle_key_binding, (void *)pointer);
		pointer->bindings[1] = nemocompz_add_touch_binding(compz, minishell_handle_touch_binding, (void *)pointer);
	} else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
		nemobinding_destroy(pointer->bindings[0]);
		nemobinding_destroy(pointer->bindings[1]);
	}
}

static void minishell_handle_touch_down(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	if (tp->focus != NULL && tp->focus->canvas != NULL) {
		struct nemocanvas *parent = nemosubcanvas_get_main_canvas(tp->focus->canvas);
		struct shellbin *bin = nemoshell_get_bin(tp->focus->canvas);

		nemoview_update_layer(tp->focus);

		if (bin != NULL) {
			datadevice_set_focus(tp->touch->seat, tp->focus);
		}
	} else if (tp->focus != NULL && tp->focus->actor != NULL) {
		nemoview_update_layer(tp->focus);
	}

	wl_signal_emit(&compz->activate_signal, tp->focus);
}

static int minishell_dispatch_move_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct minigrab *grab = (struct minigrab *)container_of(base, struct minigrab, base);
	struct minishell *mini = grab->mini;
	struct nemoshow *show = mini->show;
	struct nemoactor *actor = NEMOSHOW_AT(show, actor);

	if (type & NEMOTALE_DOWN_EVENT) {
		struct showone *group = grab->group;

		grab->dx = nemoshow_item_get_tx(group) - event->x;
		grab->dy = nemoshow_item_get_ty(group) - event->y;
	} else if (type & NEMOTALE_MOTION_EVENT) {
		struct showone *group = grab->group;

		nemoshow_item_translate(group, event->x + grab->dx, event->y + grab->dy);

		nemoactor_dispatch_frame(actor);

		grab->x = event->x;
		grab->y = event->y;
	} else if (type & NEMOTALE_UP_EVENT) {
		nemoactor_dispatch_frame(actor);

		minishell_grab_destroy(grab);

		return 0;
	}

	return 1;
}

static int minishell_dispatch_rotate_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct minigrab *grab = (struct minigrab *)container_of(base, struct minigrab, base);
	struct minishell *mini = grab->mini;
	struct nemoshow *show = mini->show;
	struct nemoactor *actor = NEMOSHOW_AT(show, actor);

	if (type & NEMOTALE_DOWN_EVENT) {
		struct showone *group = grab->group;

		grab->dx = nemoshow_item_get_tx(group) - event->x;
		grab->dy = nemoshow_item_get_ty(group) - event->y;
		grab->ro = nemoshow_item_get_rotate(group) - atan2(nemoshow_item_get_ty(group) - event->y, nemoshow_item_get_tx(group) - event->x) * 180.0f / M_PI;
	} else if (type & NEMOTALE_MOTION_EVENT) {
		struct showone *group = grab->group;

		nemoshow_item_rotate(group, grab->ro + atan2(nemoshow_item_get_ty(group) - event->y, nemoshow_item_get_tx(group) - event->x) * 180.0f / M_PI);

		nemoactor_dispatch_frame(actor);

		grab->x = event->x;
		grab->y = event->y;
	} else if (type & NEMOTALE_UP_EVENT) {
		nemoactor_dispatch_frame(actor);

		minishell_grab_destroy(grab);

		return 0;
	}

	return 1;
}

#define MINISHELL_PALM_UPDATE_INTERVAL		(300)
#define MINISHELL_PALM_UPDATE_DISTANCE		(10.0f)

static int minishell_dispatch_palm_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct minigrab *grab = (struct minigrab *)container_of(base, struct minigrab, base);
	struct minishell *mini = grab->mini;
	struct nemoshow *show = mini->show;
	struct nemoactor *actor = NEMOSHOW_AT(show, actor);

	if (type & NEMOTALE_DOWN_EVENT) {
		struct showone *group = grab->group;

		minishell_palm_prepare(mini, grab);

		grab->dx = nemoshow_item_get_tx(group) - event->x;
		grab->dy = nemoshow_item_get_ty(group) - event->y;
	} else if (type & NEMOTALE_MOTION_EVENT) {
		struct showone *group = grab->group;

		if (minishell_grab_check_update(grab, event, MINISHELL_PALM_UPDATE_INTERVAL, MINISHELL_PALM_UPDATE_DISTANCE) != 0) {
			minishell_palm_update(mini, grab);

			minishell_grab_update(grab, event);
		}

		if (grab->type == MINISHELL_NORMAL_GRAB || grab->type == MINISHELL_PALM_GRAB) {
			nemoshow_item_translate(group, event->x + grab->dx, event->y + grab->dy);
		}

		nemoactor_dispatch_frame(actor);

		grab->x = event->x;
		grab->y = event->y;
	} else if (type & NEMOTALE_UP_EVENT) {
		minishell_palm_finish(mini, grab);

		nemoactor_dispatch_frame(actor);

		minishell_grab_destroy(grab);

		return 0;
	} else if (type & NEMOTALE_LONG_PRESS_EVENT) {
	}

	return 1;
}

#define	MINISHELL_YOYO_UPDATE_INTERVAL		(300)
#define	MINISHELL_YOYO_UPDATE_DISTANCE		(10.0f)

static int minishell_dispatch_yoyo_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct minigrab *grab = (struct minigrab *)container_of(base, struct minigrab, base);
	struct minishell *mini = grab->mini;
	struct nemoshow *show = mini->show;
	struct nemoactor *actor = NEMOSHOW_AT(show, actor);

	if (type & NEMOTALE_DOWN_EVENT) {
		struct miniyoyo *yoyo = (struct miniyoyo *)grab->userdata;

		minishell_yoyo_prepare(yoyo, event->x, event->y);
	} else if (type & NEMOTALE_MOTION_EVENT) {
		if (minishell_grab_check_update(grab, event, MINISHELL_YOYO_UPDATE_INTERVAL, MINISHELL_YOYO_UPDATE_DISTANCE) != 0) {
			struct miniyoyo *yoyo = (struct miniyoyo *)grab->userdata;
			struct showone *one = grab->one;
			int32_t minx, miny, maxx, maxy;
			double outer;

			minishell_yoyo_update(yoyo, event->x, event->y, &minx, &miny, &maxx, &maxy);

			outer = nemoshow_item_get_outer(one);
			minx -= outer;
			miny -= outer;
			maxx += outer;
			maxy += outer;

			nemoshow_item_update_boundingbox(show, one);
			nemoshow_canvas_damage_region(nemoshow_item_get_canvas(one),
					minx, miny, maxx - minx, maxy - miny);

			nemoactor_dispatch_frame(actor);

			minishell_grab_update(grab, event);
		}
	} else if (type & NEMOTALE_UP_EVENT) {
		struct miniyoyo *yoyo = (struct miniyoyo *)grab->userdata;
		struct showone *one = grab->one;
		struct showtransition *trans;
		struct showone *sequence;

		minishell_yoyo_finish(yoyo);

		nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);

		sequence = nemoshow_sequence_create_easy(show,
				nemoshow_sequence_create_frame_easy(show,
					1.0f,
					nemoshow_sequence_create_set_easy(show,
						one,
						"from", "1.0",
						NULL),
					NULL),
				NULL);

		trans = nemoshow_transition_create(nemoshow_search_one(show, "ease1"), 700, 0);
		nemoshow_transition_attach_sequence(trans, sequence);
		nemoshow_attach_transition(show, trans);

		nemoactor_dispatch_frame(actor);

		minishell_yoyo_destroy(yoyo);

		minishell_grab_destroy(grab);

		return 0;
	}

	return 1;
}

static void minishell_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (nemotale_is_pointer_enter(tale, event, type)) {
		struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
		struct nemoshell *shell = NEMOSHOW_AT(show, shell);
		struct nemocompz *compz = shell->compz;
		struct nemoseat *seat = compz->seat;
		struct nemopointer *pointer;
		struct nemoactor *cursor;
		int32_t dx, dy;

		pointer = nemoseat_get_pointer_by_id(seat, event->device);
		if (pointer != NULL) {
			int32_t width = 32;
			int32_t height = 32;

			cursor = nemoactor_create_pixman(shell->compz, width, height);
			if (cursor != NULL) {
				skia_draw_circle(
						cursor->data, width, height,
						width / 2.0f, height / 2.0f, width / 3.0f,
						SkPaint::kFill_Style,
						SkColorSetARGB(255, 0, 255, 255),
						5.0f);

				nemopointer_set_cursor_actor(pointer, cursor, width / 2, height / 2);
			}
		}
	} else if (nemotale_is_pointer_leave(tale, event, type)) {
	}

	if (id == 1) {
		if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
			struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
			struct nemoactor *actor = NEMOSHOW_AT(show, actor);
			struct minishell *mini = (struct minishell *)nemoshow_get_userdata(show);
			struct showone *canvas = mini->canvas;

			if (nemotale_is_touch_down(tale, event, type)) {
				int32_t pid = nemoshow_canvas_pick_one(canvas, event->x, event->y);

				if (pid == 10 && nemobook_is_empty(mini->book, pid) == 0) {
					struct showone *one = (struct showone *)nemobook_get(mini->book, pid);
					struct showone *group = nemoshow_one_get_parent(one, NEMOSHOW_ITEM_TYPE, NEMOSHOW_GROUP_ITEM);
					struct minigrab *grab;

					grab = minishell_grab_create(mini, tale, event, minishell_dispatch_move_grab, NULL);
					grab->group = group;
					grab->one = one;
					nemotale_dispatch_grab(tale, event->device, type, event);
				} else if (pid != 0 && nemobook_is_empty(mini->book, pid) == 0) {
					struct showone *one = (struct showone *)nemobook_get(mini->book, pid);
					struct showone *group = nemoshow_one_get_parent(one, NEMOSHOW_ITEM_TYPE, NEMOSHOW_GROUP_ITEM);
					struct minigrab *grab;

					grab = minishell_grab_create(mini, tale, event, minishell_dispatch_rotate_grab, NULL);
					grab->group = group;
					grab->one = one;
					nemotale_dispatch_grab(tale, event->device, type, event);
				} else {
					struct showone *group;
					struct showone *edge;
					struct showone *one;
					struct showtransition *trans;
					struct showone *sequence;
					struct minigrab *grab;

					group = nemoshow_item_create(NEMOSHOW_GROUP_ITEM);
					nemoshow_attach_one(show, group);
					nemoshow_item_attach_one(canvas, group);
					nemoshow_item_set_tsr(group);
					nemoshow_item_translate(group, event->x, event->y);

					edge = nemoshow_item_create(NEMOSHOW_DONUT_ITEM);
					nemoshow_attach_one(show, edge);
					nemoshow_item_attach_one(group, edge);
					nemoshow_item_set_x(edge, -50.0f);
					nemoshow_item_set_y(edge, -50.0f);
					nemoshow_item_set_width(edge, 100.0f);
					nemoshow_item_set_height(edge, 100.0f);
					nemoshow_item_set_inner(edge, 5.0f);
					nemoshow_item_set_from(edge, 0.0f);
					nemoshow_item_set_to(edge, 0.0f);
					nemoshow_item_set_filter(edge, mini->blur5);
					nemoshow_item_set_fill_color(edge, 255, 255, 0, 255);

					one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
					nemoshow_attach_one(show, one);
					nemoshow_item_attach_one(group, one);
					nemoshow_item_set_x(one, 0.0f);
					nemoshow_item_set_y(one, 0.0f);
					nemoshow_item_set_r(one, 0.0f);
					nemoshow_item_set_filter(one, mini->blur15);
					nemoshow_item_set_fill_color(one, 255, 255, 0, 255);

					sequence = nemoshow_sequence_create_easy(show,
							nemoshow_sequence_create_frame_easy(show,
								0.7f,
								nemoshow_sequence_create_set_easy(show,
									one,
									"r", "40.0",
									NULL),
								NULL),
							nemoshow_sequence_create_frame_easy(show,
								1.0f,
								nemoshow_sequence_create_set_easy(show,
									edge,
									"to", "45.0",
									NULL),
								NULL),
							NULL);

					trans = nemoshow_transition_create(nemoshow_search_one(show, "ease0"), 800, 0);
					nemoshow_transition_attach_sequence(trans, sequence);
					nemoshow_attach_transition(show, trans);

					nemoactor_dispatch_frame(actor);

					grab = minishell_grab_create(mini, tale, event, minishell_dispatch_palm_grab, NULL);
					grab->group = group;
					grab->edge = edge;
					grab->one = one;
					nemotale_dispatch_grab(tale, event->device, type, event);
				}
			}

			if (nemotale_is_pointer_left_down(tale, event, type) ||
					nemotale_is_pointer_button_down(tale, event, type, BTN_0)) {
				struct showone *one;
				struct minigrab *grab;
				struct miniyoyo *yoyo;

				one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
				nemoshow_attach_one(show, one);
				nemoshow_one_attach_one(canvas, one);
				nemoshow_item_set_filter(one, mini->blur5);
				nemoshow_item_set_stroke_color(one, 255, 255, 0, 255);
				nemoshow_item_set_stroke_width(one, 3.0f);

				nemoactor_dispatch_frame(actor);

				yoyo = minishell_yoyo_create(
						nemocompz_get_scene_width(actor->compz),
						nemocompz_get_scene_height(actor->compz),
						nemoshow_item_get_skia_path(one));

				grab = minishell_grab_create(mini, tale, event, minishell_dispatch_yoyo_grab, yoyo);
				grab->type = MINISHELL_YOYO_GRAB;
				grab->one = one;
				nemotale_dispatch_grab(tale, event->device, type, event);
			}

			if (nemotale_is_pointer_axis(tale, event, type)) {
			}

			if (nemotale_is_single_click(tale, event, type)) {
				struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
				struct nemoshell *shell = NEMOSHOW_AT(show, shell);
				struct nemocompz *compz = shell->compz;
				int32_t pid = nemoshow_canvas_pick_one(canvas, event->x, event->y);

				if (pid == 77) {
					int index;

					index = nemoitem_get_ifone(shell->configs, "//nemoshell/palm/item", 0, "id", "icon0");
					if (index >= 0) {
						char *itempath;

						itempath = nemoitem_get_attr(shell->configs, index, "path");
						if (itempath != NULL) {
							struct wl_client *client;
							char *argv[2];

							argv[0] = itempath;
							argv[1] = NULL;

							client = wayland_execute_client(compz->display, argv[0], argv, NULL, NULL);
							if (client != NULL) {
								struct clientstate *state;

								state = nemoshell_create_client_state(client);
								if (state != NULL) {
									state->x = event->x;
									state->y = event->y;
									state->dx = 0.5f;
									state->dy = 0.5f;
									state->flags = NEMO_SHELL_SURFACE_ALL_FLAGS;
								}
							}
						}
					}
				} else if (pid != 0 && nemobook_is_empty(mini->book, pid) == 0) {
					struct showone *one = (struct showone *)nemobook_get(mini->book, pid);
					struct showone *group = nemoshow_one_get_parent(one, NEMOSHOW_ITEM_TYPE, NEMOSHOW_GROUP_ITEM);
					struct showone *nuts[6];
					struct showtransition *trans[6];
					struct showone *sequence;
					struct showone *set;
					struct showone *nut, *nut0;
					double x, y;
					double r;
					int i;

					r = atan2(nemoshow_item_get_ty(one), nemoshow_item_get_tx(one));

					for (i = 0, nut0 = one; i < 6; i++, nut0 = nut) {
						if ((i % 2) == 0) {
							x = cos(r + i * 15.0f * M_PI / 180.0f) * 250.0f;
							y = sin(r + i * 15.0f * M_PI / 180.0f) * 250.0f;
						} else {
							x = cos(r + i * 15.0f * M_PI / 180.0f) * 175.0f;
							y = sin(r + i * 15.0f * M_PI / 180.0f) * 175.0f;
						}

						nuts[i] = nut = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
						nemoshow_attach_one(show, nut);
						nemoshow_item_attach_one(group, nut);
						nemoshow_item_set_event(nut, 77);
						nemoshow_item_set_x(nut, cos(r) * 100.0f);
						nemoshow_item_set_y(nut, sin(r) * 100.0f);
						nemoshow_item_set_r(nut, 20.0f);
						nemoshow_item_set_filter(nut, mini->blur15);
						nemoshow_item_set_fill_color(nut, 0, 255, 255, 255);

						set = nemoshow_sequence_create_set();
						nemoshow_sequence_set_source(set, nut);
						nemoshow_sequence_set_dattr(set, "x", x, NEMOSHOW_SHAPE_DIRTY);
						nemoshow_sequence_set_dattr(set, "y", y, NEMOSHOW_SHAPE_DIRTY);

						sequence = nemoshow_sequence_create_easy(show,
								nemoshow_sequence_create_frame_easy(show, 1.0f, set, NULL),
								NULL);

						trans[i] = nemoshow_transition_create(nemoshow_search_one(show, "ease0"), 700, i * 150);
						nemoshow_transition_attach_sequence(trans[i], sequence);
						nemoshow_attach_transition(show, trans[i]);
					}

					nemoactor_dispatch_frame(actor);
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "scene",							required_argument,	NULL,		'x' },
		{ "node",								required_argument,	NULL,		'n' },
		{ "seat",								required_argument,	NULL,		's' },
		{ "tty",								required_argument,	NULL,		't' },
		{ "config",							required_argument,	NULL,		'c' },
		{ "log",								required_argument,	NULL,		'l' },
		{ "help",								no_argument,				NULL,		'h' },
		{ 0 }
	};

	struct minishell *mini;
	struct nemoshell *shell;
	struct nemocompz *compz;
	struct nemoactor *actor;
	struct nemoshow *show;
	struct showone *blur;
	char *scenexml = NULL;
	char *rendernode = NULL;
	char *configpath = NULL;
	char *seat = NULL;
	int tty = 0;
	int opt;

	nemolog_set_file(stderr);

	while (opt = getopt_long(argc, argv, "x:n:s:t:c:l:h", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'x':
				scenexml = strdup(optarg);
				break;

			case 'n':
				rendernode = strdup(optarg);
				break;

			case 's':
				seat = strdup(optarg);
				break;

			case 't':
				tty = strtoul(optarg, NULL, 10);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			case 'l':
				nemolog_open_file(optarg);
				break;

			case 'h':
				fprintf(stderr, "usage: nemoshell --use-pixman --backend [fb/drm/wayland] --node [rendernode] --seat [name] --tty [number] --config [filepath]\n");
				return 0;

			default:
				break;
		}
	}

	nemolog_message("SHELL", "start nemoshell...\n");

	if (configpath == NULL)
		asprintf(&configpath, "%s/.config/nemoshell.xml", getenv("HOME"));

	mini = (struct minishell *)malloc(sizeof(struct minishell));
	if (mini == NULL)
		return -1;
	memset(mini, 0, sizeof(struct minishell));

	mini->book = nemobook_create(256);
	if (mini->book == NULL)
		return -1;
	nemobook_iset(mini->book, 0, 0x12345678);

	nemolist_init(&mini->grab_list);

	compz = nemocompz_create();
	if (compz == NULL)
		return -1;

	mini->shell = shell = nemoshell_create(compz);
	if (shell == NULL)
		goto out;

	nemoshow_initialize();

	if (seat != NULL)
		nemosession_connect(compz->session, seat, tty);

	minishell_load_configs(shell, configpath);

#ifdef NEMOUX_WITH_XWAYLAND
	nemoxserver_create(shell,
			nemoitem_get_attr_named(shell->configs, "//nemoshell/xserver", "path"));
#endif

	if (scenexml == NULL)
		scenexml = nemoitem_get_attr_named(shell->configs, "//nemoshell/scene", "path");

	drmbackend_create(compz, rendernode);
	evdevbackend_create(compz);
	tuiobackend_create(compz);

	nemocompz_load_plugins(compz);

	minishell_load_virtuio(shell);

	nemocompz_set_screen_frame_listener(compz, nemocompz_get_main_screen(compz));

	nemocompz_add_key_binding(compz, KEY_ESC, MODIFIER_CTRL, minishell_handle_escape_key, (void *)shell);
	nemocompz_add_button_binding(compz, BTN_LEFT, minishell_handle_left_button, (void *)shell);
	nemocompz_add_button_binding(compz, BTN_RIGHT, minishell_handle_right_button, (void *)shell);
	nemocompz_add_touch_binding(compz, minishell_handle_touch_down, (void *)shell);

	nemocompz_make_current(compz);

	mini->show = show = nemoshow_create_on_actor(shell,
			nemocompz_get_scene_width(compz),
			nemocompz_get_scene_height(compz),
			minishell_dispatch_tale_event);
	if (show == NULL)
		goto out;
	nemoshow_load_xml(show, scenexml);
	nemoshow_arrange_one(show);
	nemoshow_update_one(show);
	nemoshow_set_userdata(show, mini);

	nemoshow_set_scene(show,
			nemoshow_search_one(show, "scene0"));
	nemoshow_set_size(show,
			nemocompz_get_scene_width(compz),
			nemocompz_get_scene_height(compz));

	nemoshow_render_one(show);

	mini->canvas = nemoshow_search_one(show, "canvas");

	mini->blur5 = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	mini->blur15 = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 15.0f);

	actor = NEMOSHOW_AT(show, actor);
	nemoview_attach_layer(actor->view, &shell->underlay_layer);
	nemoview_set_position(actor->view, 0.0f, 0.0f);
	nemoview_update_transform(actor->view);

	nemoactor_dispatch_frame(actor);

	minishell_launch_background(shell);
	minishell_launch_apps(shell);

	nemocompz_run(compz);

	nemoshow_destroy_on_actor(show);

	nemoshow_finalize();

	nemoshell_destroy(shell);

out:
	nemocompz_destroy(compz);

	nemolog_message("SHELL", "end nemoshell...\n");

	nemobook_destroy(mini->book);

	free(mini);

	free(configpath);

	nemolog_close_file();

	return 0;
}
