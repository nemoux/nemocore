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

#include <shell.h>
#include <compz.h>
#include <screen.h>
#include <view.h>
#include <content.h>
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
#include <nemomisc.h>

#include <nemoenvs.h>
#include <nemotoken.h>
#include <nemoitem.h>
#include <nemolog.h>
#include <pixmanhelper.h>

void nemoenvs_handle_terminal_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		struct nemoenvs *envs = (struct nemoenvs *)data;
		const char *_path = envs->terminal.path;
		const char *_args = envs->terminal.args;
		struct nemotoken *args;

		args = nemotoken_create(_path, strlen(_path));
		if (_args != NULL)
			nemotoken_append_format(args, ";%s", _args);
		nemotoken_divide(args, ';');
		nemotoken_update(args);

		os_file_execute(_path, nemotoken_get_tokens(args), NULL);

		nemotoken_destroy(args);
	}
}

void nemoenvs_handle_touch_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
}

void nemoenvs_handle_escape_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data)
{
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		nemolog_message("SHELL", "exit nemoshell by escape key\n");

		nemocompz_set_return_value(compz, 1);

		nemocompz_destroy_clients(compz);
		nemocompz_exit(compz);
	}
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
			}
		} else if (pointer->keyboard != NULL && nemoxkb_has_modifiers_state(pointer->keyboard->xkb, MODIFIER_ALT)) {
			struct nemoscreen *screen;

			screen = nemocompz_get_screen_on(compz, pointer->x, pointer->y);
			if (screen != NULL) {
				pixman_image_t *image;
				pixman_transform_t transform;

				image = pixman_image_create_bits(PIXMAN_a8r8g8b8, screen->width, screen->height, NULL, screen->width * 4);
				pixman_transform_init_identity(&transform);
				pixman_transform_translate(&transform, NULL,
						pixman_double_to_fixed(screen->width / -2.0f),
						pixman_double_to_fixed(screen->height / -2.0f));
				pixman_transform_scale(&transform, NULL,
						pixman_double_to_fixed(1.0f),
						pixman_double_to_fixed(-1.0f));
				pixman_transform_translate(&transform, NULL,
						pixman_double_to_fixed(screen->width / 2.0f),
						pixman_double_to_fixed(screen->height / 2.0f));
				pixman_image_set_transform(image, &transform);

				nemoscreen_read_pixels(screen, PIXMAN_a8r8g8b8,
						pixman_image_get_data(image),
						screen->x, screen->y,
						screen->width, screen->height);

				pixman_save_png_file(image, "nemoshot.png");

				pixman_image_unref(image);
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

static void nemoenvs_get_distant_touchpoint(struct touchpoint *tps[], int ntps, struct touchpoint **tp0, struct touchpoint **tp1)
{
	struct nemotouch *touch;
	struct touchpoint *_tp0, *_tp1;
	float dm = 0.0f;
	float dd;
	float dx, dy;
	int i, j;

	for (i = 0; i < ntps - 1; i++) {
		_tp0 = tps[i];

		for (j = i + 1; j < ntps; j++) {
			_tp1 = tps[j];

			dx = _tp1->x - _tp0->x;
			dy = _tp1->y - _tp0->y;
			dd = sqrtf(dx * dx + dy * dy);

			if (dd > dm) {
				dm = dd;

				*tp0 = _tp0;
				*tp1 = _tp1;
			}
		}
	}
}

void nemoenvs_handle_touch_event(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)data;
	struct nemoshell *shell = envs->shell;

	if (tp->state == TOUCHPOINT_DOWN_STATE) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen_on(shell, tp->x, tp->y, NEMOSHELL_FULLSCREEN_PITCH_TYPE);
		if (screen != NULL) {
			struct shellbin *sbin, *nbin;
			uint32_t target = screen->target;

			wl_list_for_each(screen, &shell->fullscreen_list, link) {
				if ((screen->target & target) == 0)
					continue;

				wl_list_for_each_safe(sbin, nbin, &screen->bin_list, screen_link) {
					wl_list_remove(&sbin->screen_link);
					wl_list_init(&sbin->screen_link);

					if (nemoshell_move_canvas_force(shell, tp, sbin) == 0)
						break;
				}
			}
		}

		if (tp->focus != NULL && tp->focus->canvas != NULL) {
			struct nemocanvas *parent = nemosubcanvas_get_main_canvas(tp->focus->canvas);
			struct shellbin *bin = nemoshell_get_bin(tp->focus->canvas);

			if (bin != NULL) {
				nemoview_above_layer(tp->focus, NULL);

				datadevice_set_focus(tp->touch->seat, tp->focus);

				if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_BINDABLE_STATE) != 0) {
					struct touchpoint *tps[10];
					int tapcount;

					tapcount = nemoseat_get_touchpoint_by_view(compz->seat, tp->focus, tps, 10);
					if (tapcount >= envs->legacy.pick_taps) {
						struct touchpoint *tp0, *tp1;

						nemoenvs_get_distant_touchpoint(tps, tapcount, &tp0, &tp1);

						if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_SCALABLE_FLAG)) {
							nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, NEMOSHELL_PICK_ROTATE_FLAG | NEMOSHELL_PICK_TRANSLATE_FLAG | NEMOSHELL_PICK_SCALE_FLAG, bin);
						} else {
							nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, NEMOSHELL_PICK_ROTATE_FLAG | NEMOSHELL_PICK_TRANSLATE_FLAG, bin);
						}
					}
				}
			}
		}

		wl_signal_emit(&compz->activate_signal, tp->focus);
	} else if (tp->state == TOUCHPOINT_UP_STATE) {
		if (tp->focus != NULL && tp->focus->canvas != NULL && touchpoint_has_flags(tp, TOUCHPOINT_GRAB_FLAG) != 0) {
			struct nemocanvas *parent = nemosubcanvas_get_main_canvas(tp->focus->canvas);
			struct shellbin *bin = nemoshell_get_bin(tp->focus->canvas);

			if (bin != NULL) {
				if (nemoshell_bin_has_state(bin, NEMOSHELL_BIN_BINDABLE_STATE) != 0) {
					struct touchpoint *tps[10];
					int tapcount;

					tapcount = nemoseat_get_touchpoint_by_view(compz->seat, tp->focus, tps, 10);
					if (tapcount >= 2) {
						struct touchpoint *tp0, *tp1;

						nemoenvs_get_distant_touchpoint(tps, tapcount, &tp0, &tp1);

						if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_SCALABLE_FLAG)) {
							nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, NEMOSHELL_PICK_ROTATE_FLAG | NEMOSHELL_PICK_TRANSLATE_FLAG | NEMOSHELL_PICK_SCALE_FLAG, bin);
						} else {
							nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, NEMOSHELL_PICK_ROTATE_FLAG | NEMOSHELL_PICK_TRANSLATE_FLAG, bin);
						}
					} else if (tapcount == 1) {
						nemoshell_move_canvas_by_touchpoint(shell, tps[0], bin);
					}
				}
			}
		}
	}
}
