#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <signal.h>
#include <linux/input.h>
#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <shell.h>
#include <compz.h>
#include <screen.h>
#include <view.h>
#include <content.h>
#include <actor.h>
#include <canvas.h>
#include <subcanvas.h>
#include <seat.h>
#include <keyboard.h>
#include <pointer.h>
#include <touch.h>
#include <virtuio.h>
#include <datadevice.h>
#include <session.h>
#include <binding.h>
#include <plugin.h>
#include <timer.h>
#include <animation.h>
#include <grab.h>
#include <move.h>
#include <pick.h>
#include <picker.h>
#include <sound.h>
#include <waylandhelper.h>
#include <nemoxml.h>
#include <nemolog.h>
#include <nemoitem.h>
#include <nemobox.h>
#include <nemomisc.h>

#include <nemoenvs.h>
#include <nemotoken.h>
#include <nemoitem.h>
#include <showhelper.h>
#include <nemohelper.h>
#include <nemolog.h>

void nemoenvs_handle_terminal_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		struct nemoenvs *envs = (struct nemoenvs *)data;
		const char *_path = nemoitem_get_sattr(envs->configs, "/nemoshell/terminal", "path", NULL);
		const char *_args = nemoitem_get_sattr(envs->configs, "/nemoshell/terminal", "args", NULL);
		struct nemotoken *args;

		args = nemotoken_create(_path, strlen(_path));
		if (_args != NULL)
			nemotoken_append_format(args, ";%s", _args);
		nemotoken_divide(args, ';');
		nemotoken_update(args);

		wayland_execute_path(_path, nemotoken_get_tokens(args), NULL);

		nemotoken_destroy(args);
	}
}

static void nemoenvs_dispatch_touch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	if (nemoshow_event_is_touch_down(show, event)) {
		struct showone *one;

		one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
		nemoshow_item_set_cx(one, nemoshow_event_get_x(event));
		nemoshow_item_set_cy(one, nemoshow_event_get_y(event));
		nemoshow_item_set_r(one, 15.0f);
		nemoshow_item_set_stroke_color(one,
				random_get_int(0, 255),
				random_get_int(0, 255),
				random_get_int(0, 255),
				255.0f);
		nemoshow_item_set_stroke_width(one, 10.0f);
		nemoshow_one_attach(canvas, one);

		nemoshow_event_set_data(event, one);
	} else if (nemoshow_event_is_touch_up(show, event)) {
		struct showone *one = (struct showone *)nemoshow_event_get_data(event);

		nemoshow_one_destroy(one);
	} else if (nemoshow_event_is_touch_motion(show, event)) {
		struct showone *one = (struct showone *)nemoshow_event_get_data(event);

		nemoshow_item_set_cx(one, nemoshow_event_get_x(event));
		nemoshow_item_set_cy(one, nemoshow_event_get_y(event));
	}

	nemoshow_dispatch_frame(show);

	if (nemoshow_event_is_keyboard_down(show, event)) {
		nemoshow_revoke_view(show);
		nemoshow_destroy_view_on_idle(show);
	}
}

void nemoenvs_handle_touch_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		struct nemoenvs *envs = (struct nemoenvs *)data;
		struct nemoshell *shell = envs->shell;
		struct nemoshow *show;
		struct showone *scene;
		struct showone *canvas;
		int width = nemocompz_get_scene_width(compz);
		int height = nemocompz_get_scene_height(compz);

		show = nemoshow_create_view(shell, width, height);
		nemoshow_view_set_position(show, 0.0f, 0.0f);
		nemoshow_view_set_layer(show, "overlay");

		scene = nemoshow_scene_create();
		nemoshow_scene_set_width(scene, width);
		nemoshow_scene_set_height(scene, height);
		nemoshow_set_scene(show, scene);

		canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
		nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
		nemoshow_canvas_set_alpha(canvas, 1.0f);
		nemoshow_one_attach(scene, canvas);

		canvas = nemoshow_canvas_create();
		nemoshow_canvas_set_width(canvas, width);
		nemoshow_canvas_set_height(canvas, height);
		nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
		nemoshow_canvas_set_dispatch_event(canvas, nemoenvs_dispatch_touch_canvas_event);
		nemoshow_one_attach(scene, canvas);

		nemoshow_set_keyboard_focus(show, canvas);

		nemoshow_dispatch_frame(show);
	}
}

void nemoenvs_handle_escape_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		nemolog_message("SHELL", "exit nemoshell by escape key\n");

		nemocompz_destroy_clients(compz);
		nemocompz_exit(compz);
	}
}

struct nemomirror {
	struct wl_listener canvas_damage_listener;
	struct wl_listener canvas_destroy_listener;

	struct nemoshow *show;
	struct showone *one;

	struct nemocompz *compz;
	struct nemoview *view;
};

static void nemoenvs_dispatch_mirror_show_event(struct nemoshow *show, struct showevent *event)
{
	struct nemomirror *mirror = (struct nemomirror *)nemoshow_get_userdata(show);
	struct nemocompz *compz = mirror->compz;

	if (nemoshow_event_is_pointer_enter(show, event)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_enter(pointer, mirror->view->content);
		}
	} else if (nemoshow_event_is_pointer_leave(show, event)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_leave(pointer, mirror->view->content);
		}
	}
}

static void nemoenvs_dispatch_mirror_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemomirror *mirror = (struct nemomirror *)nemoshow_get_userdata(show);
	struct nemocompz *compz = mirror->compz;

	if (nemoshow_event_is_keyboard_down(show, event)) {
		nemoshow_revoke_view(show);
		nemoshow_destroy_view_on_idle(show);
	}

	if (nemoshow_event_is_touch_down(show, event)) {
		struct touchpoint *tp;

		tp = nemoseat_get_touchpoint_by_id_nocheck(compz->seat, nemoshow_event_get_device(event));
		if (tp != NULL) {
			nemocontent_touch_down(tp, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_device(event),
					nemoshow_event_get_x(event) * mirror->view->content->width / nemoshow_canvas_get_width(mirror->one),
					nemoshow_event_get_y(event) * mirror->view->content->height / nemoshow_canvas_get_height(mirror->one),
					nemoshow_event_get_gx(event),
					nemoshow_event_get_gy(event));
		}
	} else if (nemoshow_event_is_touch_up(show, event)) {
		struct touchpoint *tp;

		tp = nemoseat_get_touchpoint_by_id_nocheck(compz->seat, nemoshow_event_get_device(event));
		if (tp != NULL) {
			nemocontent_touch_up(tp, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_device(event));
		}
	} else if (nemoshow_event_is_touch_motion(show, event)) {
		struct touchpoint *tp;

		tp = nemoseat_get_touchpoint_by_id_nocheck(compz->seat, nemoshow_event_get_device(event));
		if (tp != NULL) {
			nemocontent_touch_motion(tp, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_device(event),
					nemoshow_event_get_x(event) * mirror->view->content->width / nemoshow_canvas_get_width(mirror->one),
					nemoshow_event_get_y(event) * mirror->view->content->height / nemoshow_canvas_get_height(mirror->one),
					nemoshow_event_get_gx(event),
					nemoshow_event_get_gy(event));
		}
	}

	if (nemoshow_event_is_pointer_button_down(show, event, 0)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_button(pointer, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_value(event),
					WL_POINTER_BUTTON_STATE_PRESSED);
		}
	} else if (nemoshow_event_is_pointer_button_up(show, event, 0)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_button(pointer, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_value(event),
					WL_POINTER_BUTTON_STATE_RELEASED);
		}
	} else if (nemoshow_event_is_pointer_motion(show, event)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_motion(pointer, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_x(event) * mirror->view->content->width / nemoshow_canvas_get_width(mirror->one),
					nemoshow_event_get_y(event) * mirror->view->content->height / nemoshow_canvas_get_height(mirror->one));
		}
	}
}

static void nemoenvs_handle_mirror_damage(struct wl_listener *listener, void *data)
{
	struct nemomirror *mirror = (struct nemomirror *)container_of(listener, struct nemomirror, canvas_damage_listener);

	nemoshow_canvas_damage_below(mirror->one);
	nemoshow_dispatch_frame(mirror->show);
}

static void nemoenvs_handle_mirror_destroy(struct wl_listener *listener, void *data)
{
	struct nemomirror *mirror = (struct nemomirror *)container_of(listener, struct nemomirror, canvas_destroy_listener);
	struct nemoshow *show = mirror->show;

	nemoshow_revoke_view(show);
	nemoshow_destroy_view_on_idle(show);

	wl_list_remove(&mirror->canvas_damage_listener.link);
	wl_list_remove(&mirror->canvas_destroy_listener.link);

	free(mirror);
}

void nemoenvs_handle_left_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data)
{
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		if (pointer->focus != NULL) {
			nemoview_above_layer(pointer->focus, NULL);

			if (pointer->focus->canvas != NULL) {
				struct nemocanvas *canvas = pointer->focus->canvas;
				struct nemocanvas *parent = nemosubcanvas_get_main_canvas(canvas);
				struct shellbin *bin = nemoshell_get_bin(canvas);

				if (bin != NULL) {
					nemopointer_set_keyboard_focus(pointer, pointer->focus);
					datadevice_set_focus(pointer->seat, pointer->focus);
				}
			} else if (pointer->focus->actor != NULL) {
				nemopointer_set_keyboard_focus(pointer, pointer->focus);
			}
		}

		if (pointer->keyboard != NULL && nemoxkb_has_modifiers_state(pointer->keyboard->xkb, MODIFIER_CTRL)) {
			if (pointer->focus != NULL && pointer->focus->canvas != NULL) {
				struct nemocanvas *canvas = pointer->focus->canvas;
				pixman_image_t *image;

				image = pixman_image_create_bits(PIXMAN_a8r8g8b8, canvas->base.width, canvas->base.height, NULL, canvas->base.width * 4);

				nemocontent_read_pixels(&canvas->base, PIXMAN_a8r8g8b8, pixman_image_get_data(image));

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
			} else if (pointer->focus != NULL && pointer->focus->actor != NULL) {
				struct nemoactor *actor = pointer->focus->actor;
				pixman_image_t *image;

				image = pixman_image_create_bits(PIXMAN_a8r8g8b8, actor->base.width, actor->base.height, NULL, actor->base.width * 4);

				nemocontent_read_pixels(&actor->base, PIXMAN_a8r8g8b8, pixman_image_get_data(image));

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
			}
		} else if (pointer->keyboard != NULL && nemoxkb_has_modifiers_state(pointer->keyboard->xkb, MODIFIER_ALT)) {
			struct nemoscreen *screen;

			screen = nemocompz_get_screen_on(compz, pointer->x, pointer->y);
			if (screen != NULL) {
				pixman_image_t *image;

				image = pixman_image_create_bits(PIXMAN_a8b8g8r8, screen->width, screen->height, NULL, screen->width * 4);

				nemoscreen_read_pixels(screen, PIXMAN_a8b8g8r8,
						pixman_image_get_data(image),
						screen->x, screen->y,
						screen->width, screen->height);

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
			}
		} else if (pointer->keyboard != NULL && nemoxkb_has_modifiers_state(pointer->keyboard->xkb, MODIFIER_CTRL | MODIFIER_ALT)) {
			struct nemoenvs *envs = (struct nemoenvs *)data;
			struct nemoshell *shell = envs->shell;
			struct shellscreen *screen;
			struct nemoview *view;
			float sx, sy;

			view = nemocompz_pick_view(compz, pointer->x, pointer->y, &sx, &sy, NEMOVIEW_PICK_STATE);
			screen = nemoshell_get_fullscreen(shell, "fullscreen-mirror");

			if (view != NULL && view->canvas != NULL && screen != NULL) {
				struct nemoshow *show;
				struct showone *scene;
				struct showone *canvas;
				struct nemomirror *mirror;
				int x = screen->dx;
				int y = screen->dy;
				int width = screen->dw;
				int height = screen->dh;

				show = nemoshow_create_view(shell, width, height);
				nemoshow_set_dispatch_event(show, nemoenvs_dispatch_mirror_show_event);
				nemoshow_view_set_position(show, x, y);
				nemoshow_view_set_layer(show, "overlay");

				scene = nemoshow_scene_create();
				nemoshow_scene_set_width(scene, width);
				nemoshow_scene_set_height(scene, height);
				nemoshow_set_scene(show, scene);

				canvas = nemoshow_canvas_create();
				nemoshow_canvas_set_width(canvas, width);
				nemoshow_canvas_set_height(canvas, height);
				nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
				nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
				nemoshow_canvas_set_alpha(canvas, 1.0f);
				nemoshow_one_attach(scene, canvas);

				canvas = nemoshow_canvas_create();
				nemoshow_canvas_set_width(canvas, width);
				nemoshow_canvas_set_height(canvas, height);
				nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
				nemoshow_canvas_set_texture(canvas, nemocanvas_get_opengl_texture(view->canvas, 0));
				nemoshow_canvas_set_dispatch_event(canvas, nemoenvs_dispatch_mirror_canvas_event);
				nemoshow_one_attach(scene, canvas);

				mirror = (struct nemomirror *)malloc(sizeof(struct nemomirror));
				mirror->compz = compz;
				mirror->view = view;
				mirror->show = show;
				mirror->one = canvas;
				mirror->canvas_damage_listener.notify = nemoenvs_handle_mirror_damage;
				wl_signal_add(&view->canvas->damage_signal, &mirror->canvas_damage_listener);
				mirror->canvas_destroy_listener.notify = nemoenvs_handle_mirror_destroy;
				wl_signal_add(&view->canvas->destroy_signal, &mirror->canvas_destroy_listener);

				nemoshow_set_keyboard_focus(show, canvas);
				nemoshow_set_userdata(show, mirror);
				nemoshow_dispatch_frame(show);
			}
		}

		wl_signal_emit(&compz->activate_signal, pointer->focus);
	}
}

static void nemoenvs_handle_touch_binding(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	struct nemopointer *pointer = (struct nemopointer *)data;
	struct nemoscreen *screen;

	screen = nemocompz_get_screen_on(compz, pointer->x, pointer->y);

	nemoinput_set_screen(tp->touch->node, screen);

	nemolog_message("SHELL", "bind touch(%s) to screen(%d:%d)\n", tp->touch->node->devnode, screen->node->nodeid, screen->screenid);
}

static void nemoenvs_handle_key_binding(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		struct nemopointer *pointer = (struct nemopointer *)data;

		nemopointer_set_keyboard(pointer, keyboard);

		nemolog_message("SHELL", "bind keyboard(%s) to pointer(%s)\n", keyboard->node->devnode, pointer->node->devnode);
	}
}

void nemoenvs_handle_right_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data)
{
	if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
		pointer->bindings[0] = nemocompz_add_key_binding(compz, KEY_ENTER, 0, nemoenvs_handle_key_binding, (void *)pointer);
	} else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
		if (pointer->bindings[0] != NULL) {
			nemobinding_destroy(pointer->bindings[0]);

			pointer->bindings[0] = NULL;
		}
	}
}

void nemoenvs_handle_touch_event(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	if (tp->focus != NULL) {
		nemoview_above_layer(tp->focus, NULL);

		if (tp->focus->canvas != NULL) {
			struct nemocanvas *parent = nemosubcanvas_get_main_canvas(tp->focus->canvas);
			struct shellbin *bin = nemoshell_get_bin(tp->focus->canvas);

			if (bin != NULL) {
				datadevice_set_focus(tp->touch->seat, tp->focus);

				if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_BINDABLE_STATE) != 0) {
					struct nemoenvs *envs = (struct nemoenvs *)data;
					struct touchpoint *tps[10];
					int tapcount;

					tapcount = nemoseat_get_touchpoint_by_view(compz->seat, tp->focus, tps, 10);
					if (tapcount >= envs->legacy.pick_taps) {
						struct touchpoint *tp0, *tp1;

						nemoseat_get_distant_touchpoint(compz->seat, tps, tapcount, &tp0, &tp1);

						if (bin->flags & NEMOSHELL_SURFACE_SCALABLE_FLAG) {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALE), bin);
						} else {
							nemoshell_pick_canvas_by_touchpoint(bin->shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE), bin);
						}
					}
				}
			}
		}
	}

	wl_signal_emit(&compz->activate_signal, tp->focus);
}
