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
#include <fbbackend.h>
#include <evdevbackend.h>
#include <tuiobackend.h>
#include <waylandhelper.h>
#include <talehelper.h>
#include <nemoxml.h>
#include <nemoitem.h>
#include <nemomisc.h>

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
				strcmp(node->name, "xserver") == 0) {
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

static void minishell_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (type & NEMOTALE_POINTER_ENTER_EVENT) {
		struct nemoshell *shell = (struct nemoshell *)nemotale_get_userdata(tale);
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
	} else if (type & NEMOTALE_POINTER_LEAVE_EVENT) {
	}
}

static void minishell_dispatch_background_frame(struct nemoactor *actor, uint32_t msecs)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemocompz *compz = actor->compz;
	pixman_region32_t region;

	pixman_region32_init(&region);

	nemocompz_make_current(compz);

	if (msecs == 0) {
		nemoactor_feedback(actor);
	} else {
		nemoactor_feedback_done(actor);
	}

	nemotale_composite(tale, &region);

	nemoactor_damage_region(actor, &region);

	pixman_region32_fini(&region);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "use_pixman",					no_argument,				NULL,		'p' },
		{ "use_pixman_shell",		no_argument,				NULL,		'x' },
		{ "backend",						required_argument,	NULL,		'b' },
		{ "node",								required_argument,	NULL,		'n' },
		{ "seat",								required_argument,	NULL,		's' },
		{ "tty",								required_argument,	NULL,		't' },
		{ "config",							required_argument,	NULL,		'c' },
		{ "log",								required_argument,	NULL,		'l' },
		{ "help",								no_argument,				NULL,		'h' },
		{ 0 }
	};

	struct nemoshell *shell;
	struct nemocompz *compz;
	struct nemoactor *actor;
	struct nemotale *tale;
	struct talenode *node;
	struct pathone *group;
	struct pathone *back;
	char *backend = NULL;
	char *rendernode = NULL;
	char *configpath = NULL;
	char *seat = NULL;
	int use_pixman = 0;
	int use_pixman_shell = 0;
	int tty = 0;
	int opt;

	nemolog_set_file(stderr);

	while (opt = getopt_long(argc, argv, "pxb:n:s:t:c:l:h", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'p':
				use_pixman = 1;
				break;

			case 'x':
				use_pixman_shell = 1;
				break;

			case 'b':
				backend = strdup(optarg);
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

	compz = nemocompz_create();
	if (compz == NULL)
		return -1;

	shell = nemoshell_create(compz);
	if (shell == NULL)
		goto out;

	compz->use_pixman = use_pixman;

	if (seat != NULL)
		nemosession_connect(compz->session, seat, tty);

	minishell_handle_configs(shell, configpath);

	if (backend == NULL || strcmp(backend, "drm") == 0) {
		drmbackend_create(compz, rendernode);
		evdevbackend_create(compz);
		tuiobackend_create(compz);
	} else if (strcmp(backend, "fb") == 0) {
		fbbackend_create(compz, rendernode);
		evdevbackend_create(compz);
		tuiobackend_create(compz);

		compz->use_pixman = 1;
	}

	nemocompz_set_screen_frame_listener(compz, nemocompz_get_main_screen(compz));

	nemocompz_add_key_binding(compz, KEY_ESC, MODIFIER_CTRL, minishell_handle_escape_key, (void *)shell);
	nemocompz_add_button_binding(compz, BTN_LEFT, minishell_handle_left_button, (void *)shell);
	nemocompz_add_button_binding(compz, BTN_RIGHT, minishell_handle_right_button, (void *)shell);
	nemocompz_add_touch_binding(compz, minishell_handle_touch_down, (void *)shell);

	if (use_pixman != 0 || use_pixman_shell != 0) {
		actor = nemoactor_create_pixman(compz,
				nemocompz_get_scene_width(compz),
				nemocompz_get_scene_height(compz));

		tale = nemotale_create_pixman();
		nemotale_attach_pixman(tale,
				pixman_image_get_data(actor->image),
				pixman_image_get_width(actor->image),
				pixman_image_get_height(actor->image),
				pixman_image_get_stride(actor->image));
	} else {
		nemocompz_make_current(compz);

		actor = nemoactor_create_gl(compz,
				nemocompz_get_scene_width(compz),
				nemocompz_get_scene_height(compz));

		tale = nemotale_create_fbo();
		nemotale_attach_fbo(tale, actor->texture, actor->base.width, actor->base.height);
	}

	nemoactor_set_dispatch_frame(actor, minishell_dispatch_background_frame);

	nemotale_attach_actor(tale, actor, minishell_dispatch_tale_event);
	nemotale_set_userdata(tale, shell);

	nemoview_attach_layer(actor->view, &shell->background_layer);
	nemoview_set_position(actor->view, 0.0f, 0.0f);
	nemoview_update_transform(actor->view);

	node = nemotale_node_create_pixman(actor->base.width, actor->base.height);
	nemotale_node_fill_pixman(node, 0.0f, 0.3f, 0.3f, 1.0f);
	nemotale_node_set_id(node, 1);
	nemotale_node_translate(node, 0.0f, 0.0f);
	nemotale_attach_node(tale, node);

	nemoactor_dispatch_frame(actor);

	minishell_launch_apps(shell);

	nemocompz_run(compz);

	nemoactor_destroy(actor);

	nemoshell_destroy(shell);

out:
	nemocompz_destroy(compz);

	nemolog_message("SHELL", "end nemoshell...\n");

	free(configpath);

	nemolog_close_file();

	return 0;
}
