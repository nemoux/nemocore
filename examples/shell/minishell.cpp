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
#include <session.h>
#include <timer.h>
#include <binding.h>
#include <datadevice.h>
#include <drmbackend.h>
#include <fbbackend.h>
#include <evdevbackend.h>
#include <tuiobackend.h>
#include <waylandhelper.h>
#include <geometryhelper.h>
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

#include <showitem.h>
#include <showitem.hpp>

#ifdef NEMOUX_WITH_XWAYLAND
#include <xserver.h>
#endif

static void minishell_handle_configs(struct nemoshell *shell, const char *configpath)
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
				strcmp(node->name, "input") == 0) {
			i = nemoitem_set(compz->configs, node->path);

			for (j = 0; j < node->nattrs; j++) {
				nemoitem_set_attr(compz->configs, i,
						node->attrs[j*2+0],
						node->attrs[j*2+1]);
			}
		}
	}

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->name, "start") == 0 ||
				strcmp(node->name, "item") == 0 ||
				strcmp(node->name, "virtualkeyboard") == 0 ||
				strcmp(node->name, "xserver") == 0 ||
				strcmp(node->name, "scene") == 0) {
			i = nemoitem_set(shell->configs, node->path);

			for (j = 0; j < node->nattrs; j++) {
				nemoitem_set_attr(shell->configs, i,
						node->attrs[j*2+0],
						node->attrs[j*2+1]);
			}
		}
	}

	nemoxml_destroy(xml);
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

struct nemoactor *minishell_create_cursor(struct nemoshell *shell, int width, int height, int32_t *dx, int32_t *dy)
{
	struct nemoactor *actor;
	cairo_t *cr;

	actor = nemoactor_create_pixman(shell->compz, width, height);
	if (actor == NULL)
		return NULL;

	cr = cairo_create(actor->surface);

	cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cr);

	cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
	cairo_arc(cr, width * 0.5f, height * 0.5f, width * 0.4f, 0.0f, M_PI * 2.0f);
	cairo_set_source_rgba(cr, 0.0f, 1.0f, 1.0f, 1.0f);
	cairo_fill(cr);

	cairo_destroy(cr);

	*dx = width / 2;
	*dy = height / 2;

	return actor;
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

		grab->dx = NEMOSHOW_ITEM_AT(group, tx) - grab->x;
		grab->dy = NEMOSHOW_ITEM_AT(group, ty) - grab->y;
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
			nemoshow_canvas_damage_region(NEMOSHOW_ITEM_AT(one, canvas),
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
			cursor = minishell_create_cursor(shell, 16, 16, &dx, &dy);
			if (cursor != NULL)
				nemopointer_set_cursor_actor(pointer, cursor, dx, dy);
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
				struct showone *group;
				struct showone *edge;
				struct showone *one;
				struct showtransition *trans;
				struct showone *sequence;
				struct minigrab *grab;

				group = nemoshow_item_create(NEMOSHOW_GROUP_ITEM);
				nemoshow_attach_one(show, group);
				nemoshow_one_attach_one(canvas, group);
				nemoshow_item_set_canvas(group, canvas);
				nemoshow_item_set_tsr(group);
				nemoshow_item_translate(group, event->x, event->y);

				edge = nemoshow_item_create(NEMOSHOW_DONUT_ITEM);
				nemoshow_attach_one(show, edge);
				nemoshow_item_attach_one(group, edge);
				nemoshow_item_set_canvas(edge, canvas);
				NEMOSHOW_ITEM_AT(edge, x) = -50.0f;
				NEMOSHOW_ITEM_AT(edge, y) = -50.0f;
				NEMOSHOW_ITEM_AT(edge, width) = 100.0f;
				NEMOSHOW_ITEM_AT(edge, height) = 100.0f;
				NEMOSHOW_ITEM_AT(edge, inner) = 5.0f;
				NEMOSHOW_ITEM_AT(edge, from) = 0.0f;
				NEMOSHOW_ITEM_AT(edge, to) = 0.0f;
				nemoshow_item_set_blur(edge, mini->blur5);
				nemoshow_item_set_fill_color(edge, 255, 255, 0, 255);

				one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
				nemoshow_attach_one(show, one);
				nemoshow_item_attach_one(group, one);
				nemoshow_item_set_canvas(one, canvas);
				NEMOSHOW_ITEM_AT(one, x) = 0.0f;
				NEMOSHOW_ITEM_AT(one, y) = 0.0f;
				NEMOSHOW_ITEM_AT(one, r) = 0.0f;
				nemoshow_item_set_blur(one, mini->blur15);
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

				sequence = nemoshow_sequence_create_easy(show,
						nemoshow_sequence_create_frame_easy(show,
							0.5f,
							nemoshow_sequence_create_set_easy(show,
								group,
								"ro", "360.0",
								NULL),
							NULL),
						nemoshow_sequence_create_frame_easy(show,
							1.0f,
							nemoshow_sequence_create_set_easy(show,
								group,
								"ro", "0.0",
								NULL),
							NULL),
						NULL);

				trans = nemoshow_transition_create(nemoshow_search_one(show, "ease2"), 5000, 0);
				nemoshow_transition_set_repeat(trans, 0);
				nemoshow_transition_attach_sequence(trans, sequence);
				nemoshow_attach_transition(show, trans);

				nemoactor_dispatch_frame(actor);

				grab = minishell_grab_create(mini, tale, event, minishell_dispatch_palm_grab, NULL);
				grab->group = group;
				grab->edge = edge;
				grab->one = one;
				grab->trans = trans;
				nemotale_dispatch_grab(tale, event->device, type, event);
			}

			if (nemotale_is_pointer_left_down(tale, event, type) ||
					nemotale_is_pointer_button_down(tale, event, type, BTN_0)) {
				struct showone *one;
				struct minigrab *grab;
				struct miniyoyo *yoyo;

				one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
				nemoshow_attach_one(show, one);
				nemoshow_one_attach_one(canvas, one);
				nemoshow_item_set_canvas(one, canvas);
				nemoshow_item_set_blur(one, mini->blur5);
				nemoshow_item_set_stroke_color(one, 255, 255, 0, 255);
				nemoshow_item_set_stroke_width(one, 3.0f);

				nemoactor_dispatch_frame(actor);

				yoyo = minishell_yoyo_create(
						nemocompz_get_scene_width(actor->compz),
						nemocompz_get_scene_height(actor->compz),
						NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(one), path));

				grab = minishell_grab_create(mini, tale, event, minishell_dispatch_yoyo_grab, yoyo);
				grab->type = MINISHELL_YOYO_GRAB;
				grab->one = one;
				nemotale_dispatch_grab(tale, event->device, type, event);
			}

			if (nemotale_is_pointer_axis(tale, event, type)) {
			}

			if (nemotale_is_single_click(tale, event, type)) {
				int32_t pid = nemoshow_canvas_pick_one(canvas, event->x, event->y);

				if (pid != 0 && minishell_has_slot(mini, pid) != 0) {
					struct showone *one = (struct showone *)minishell_get_slot(mini, pid);
					struct showone *group = nemoshow_one_get_parent(one, NEMOSHOW_ITEM_TYPE, NEMOSHOW_GROUP_ITEM);
					struct showone *nuts[6];
					struct showtransition *trans[6];
					struct showone *sequence;
					struct showone *set;
					struct showone *link;
					struct showone *nut, *nut0;
					double x, y;
					double r;
					int i;

					r = atan2(NEMOSHOW_ITEM_AT(one, ty), NEMOSHOW_ITEM_AT(one, tx));

					for (i = 0, nut0 = one; i < 6; i++, nut0 = nut) {
						if ((i % 2) == 0) {
							x = cos(r + i * 15.0f * M_PI / 180.0f) * 200.0f;
							y = sin(r + i * 15.0f * M_PI / 180.0f) * 200.0f;
						} else {
							x = cos(r + i * 15.0f * M_PI / 180.0f) * 125.0f;
							y = sin(r + i * 15.0f * M_PI / 180.0f) * 125.0f;
						}

						nuts[i] = nut = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
						nemoshow_attach_one(show, nut);
						nemoshow_item_attach_one(group, nut);
						nemoshow_item_set_canvas(nut, canvas);
						NEMOSHOW_ITEM_AT(nut, x) = cos(r) * 50.0f;
						NEMOSHOW_ITEM_AT(nut, y) = sin(r) * 50.0f;
						NEMOSHOW_ITEM_AT(nut, r) = 20.0f;
						nemoshow_item_set_blur(nut, mini->blur15);
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

						link = nemoshow_link_create();
						nemoshow_attach_one(show, link);
						nemoshow_one_attach_one(mini->links, link);
						nemoshow_link_set_head(link, nut0);
						nemoshow_link_set_tail(link, nut);
						nemoshow_link_set_stroke_color(link, 0, 255, 255, 255);
						nemoshow_link_set_stroke_width(link, 2.0f);
						nemoshow_link_set_blur(link, mini->blur5);
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

	mini->slots = (void **)malloc(sizeof(void *) * 256);
	if (mini->slots == NULL)
		return -1;
	memset(mini->slots, 0, sizeof(void *) * 256);

	mini->nslots = 256;

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

	minishell_handle_configs(shell, configpath);

#ifdef NEMOUX_WITH_XWAYLAND
	nemoxserver_create(shell,
			nemoitem_get_attr_named(shell->configs, "//nemoshell/xserver", "path"));
#endif

	if (scenexml == NULL)
		scenexml = nemoitem_get_attr_named(shell->configs, "//nemoshell/scene", "path");

	drmbackend_create(compz, rendernode);
	evdevbackend_create(compz);
	tuiobackend_create(compz);

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

	mini->canvas = nemoshow_search_one(show, "mini");
	mini->links = nemoshow_search_one(show, "links");

	mini->blur5 = blur = nemoshow_blur_create();
	nemoshow_attach_one(show, blur);
	nemoshow_blur_set_filter(blur, "high", "solid", 5.0f);

	mini->blur15 = blur = nemoshow_blur_create();
	nemoshow_attach_one(show, blur);
	nemoshow_blur_set_filter(blur, "high", "solid", 15.0f);

	actor = NEMOSHOW_AT(show, actor);
	nemoview_attach_layer(actor->view, &shell->background_layer);
	nemoview_set_position(actor->view, 0.0f, 0.0f);
	nemoview_update_transform(actor->view);

	nemoactor_dispatch_frame(actor);

	minishell_launch_apps(shell);

	nemocompz_run(compz);

	nemoshow_destroy_on_actor(show);

	nemoshow_finalize();

	nemoshell_destroy(shell);

out:
	nemocompz_destroy(compz);

	nemolog_message("SHELL", "end nemoshell...\n");

	free(mini->slots);
	free(mini);

	free(configpath);

	nemolog_close_file();

	return 0;
}
