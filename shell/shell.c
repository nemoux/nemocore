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
#include <viewanimation.h>
#include <busycursor.h>
#include <vieweffect.h>
#include <waylandshell.h>
#include <xdgshell.h>
#include <nemoshell.h>
#include <syshelper.h>
#include <nemotoken.h>
#include <nemoitem.h>
#include <nemomisc.h>
#include <nemolog.h>

static int nemoshell_dispatch_ping_timeout(void *data)
{
	struct shellclient *sc = (struct shellclient *)data;
	struct nemoseat *seat = sc->shell->compz->seat;
	struct nemopointer *pointer;
	struct shellbin *bin;

	sc->unresponsive = 1;

	wl_list_for_each(pointer, &seat->pointer.device_list, link) {
		if (pointer->focus == NULL ||
				pointer->focus->canvas == NULL ||
				pointer->focus->canvas->resource == NULL)
			continue;

		bin = nemoshell_get_bin(pointer->focus->canvas);
		if (bin && wl_resource_get_client(bin->resource) == sc->client) {
			nemoshell_start_busycursor_grab(bin, pointer);
		}
	}

	return 1;
}

void nemoshell_ping(struct shellbin *bin, uint32_t serial)
{
	struct wl_client *client;
	struct shellclient *sc;

	if (bin == NULL || bin->resource == NULL)
		return;

	client = wl_resource_get_client(bin->resource);

	sc = nemoshell_get_client(client);
	if (sc->unresponsive) {
		nemoshell_dispatch_ping_timeout(sc);
		return;
	}

	sc->ping_serial = serial;

	if (sc->ping_timer == NULL) {
		sc->ping_timer = wl_event_loop_add_timer(bin->canvas->compz->loop, nemoshell_dispatch_ping_timeout, sc);
		if (sc->ping_timer == NULL)
			return;
	}

	wl_event_source_timer_update(sc->ping_timer, 200);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin))
		xdg_shell_send_ping(sc->resource, serial);
	else if (waylandshell_is_shell_surface(bin))
		wl_shell_surface_send_ping(bin->resource, serial);
	else if (nemoshell_is_nemo_surface(bin))
		nemo_shell_send_ping(sc->resource, serial);
}

void nemoshell_pong(struct shellclient *sc, uint32_t serial)
{
	if (sc->ping_serial != serial)
		return;

	sc->unresponsive = 0;

	nemoshell_end_busycursor_grab(sc->shell->compz, sc->client);

	if (sc->ping_timer != NULL) {
		wl_event_source_remove(sc->ping_timer);
		sc->ping_timer = NULL;
	}
}

static void shellclient_handle_client_destroy(struct wl_listener *listener, void *data)
{
	struct shellclient *sc = (struct shellclient *)container_of(listener, struct shellclient, destroy_listener);

	nemoshell_destroy_client(sc);
}

struct shellclient *nemoshell_create_client(struct wl_client *client, struct nemoshell *shell, const struct wl_interface *interface, uint32_t id)
{
	struct shellclient *sc;

	sc = (struct shellclient *)malloc(sizeof(struct shellclient));
	if (sc == NULL)
		return NULL;
	memset(sc, 0, sizeof(struct shellclient));

	sc->resource = wl_resource_create(client, interface, 1, id);
	if (sc->resource == NULL)
		goto err1;

	sc->client = client;
	sc->shell = shell;
	sc->destroy_listener.notify = shellclient_handle_client_destroy;
	wl_client_add_destroy_listener(client, &sc->destroy_listener);

	return sc;

err1:
	free(sc);

	return NULL;
}

void nemoshell_destroy_client(struct shellclient *sc)
{
	if (sc->ping_timer != NULL)
		wl_event_source_remove(sc->ping_timer);

	free(sc);
}

struct shellclient *nemoshell_get_client(struct wl_client *client)
{
	struct wl_listener *listener;

	listener = wl_client_get_destroy_listener(client, shellclient_handle_client_destroy);
	if (listener == NULL)
		return NULL;

	return (struct shellclient *)container_of(listener, struct shellclient, destroy_listener);
}

static void shellbin_configure_canvas(struct nemocanvas *canvas, int32_t sx, int32_t sy)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);
	struct nemoview *view = bin->view;
	int config_changed = 0;

	assert(bin);

	if (canvas->base.width == 0)
		return;

	if (bin->has_next_geometry != 0) {
		bin->geometry = bin->next_geometry;
		bin->has_next_geometry = 0;
		bin->has_set_geometry = 1;
	}

	if (bin->config_changed != 0) {
		if (xdgshell_is_xdg_surface(bin) || waylandshell_is_shell_surface(bin)) {
			nemoshell_change_bin_config(bin);
		} else {
			bin->config = bin->next_config;
			bin->config_changed = 0;
		}

		config_changed = 1;
	}

	if (!nemoview_has_state(view, NEMOVIEW_MAP_STATE)) {
		nemoshell_use_client_state(bin->shell, bin);

		if (bin->initial.has_position == 0) {
			pixman_box32_t *extents = pixman_region32_extents(&bin->shell->compz->scope);
			uint32_t dx = 0, dy = 0;

			if (extents->x2 - extents->x1 > canvas->base.width && extents->y2 - extents->y1 > canvas->base.height) {
				dx = canvas->base.width / 2;
				dy = canvas->base.height / 2;
			}

			bin->initial.x = random_get_int(extents->x1 + dx, extents->x2 - dx);
			bin->initial.y = random_get_int(extents->y1 + dy, extents->y2 - dy);
		}

		if (nemoview_has_state(view, NEMOVIEW_STAGE_STATE) != 0)
			nemoshell_use_client_stage(bin->shell, bin);

		if (bin->type == NEMOSHELL_SURFACE_NORMAL_TYPE) {
			if (bin->has_screen != 0) {
				nemoview_correct_pivot(view, bin->screen.width / 2.0f, bin->screen.height / 2.0f);
				nemoview_set_position(view, bin->screen.x, bin->screen.y);
				nemoview_set_rotation(view, bin->screen.r);

				nemoview_attach_layer(view, bin->layer);
				nemoview_update_transform(view);
				nemoview_damage_below(view);
			} else {
				if (view->geometry.has_anchor != 0) {
					nemoview_set_position(view,
							bin->initial.x + canvas->base.width * view->geometry.ax,
							bin->initial.y + canvas->base.height * view->geometry.ay);

					if (bin->view->geometry.has_pivot == 0)
						nemoview_correct_pivot(bin->view, view->content->width * -view->geometry.ax, view->content->height * -view->geometry.ay);
				} else {
					nemoview_set_position(view,
							bin->initial.x - canvas->base.width * bin->initial.dx,
							bin->initial.y - canvas->base.height * bin->initial.dy);

					if (bin->view->geometry.has_pivot == 0)
						nemoview_correct_pivot(bin->view, view->content->width * bin->initial.dx, view->content->height * bin->initial.dy);
				}

				nemoview_set_rotation(view, bin->initial.r);
				nemoview_set_scale(view, bin->initial.sx, bin->initial.sy);

				nemoview_attach_layer(view, bin->layer);
				nemoview_update_transform(view);
				nemoview_damage_below(view);
			}
		} else if (bin->type == NEMOSHELL_SURFACE_POPUP_TYPE) {
			struct shellbin *child;

			nemoview_update_transform(view);
			nemoview_damage_below(view);

			wl_list_for_each_reverse(child, &bin->children_list, children_link) {
				nemoview_above_layer(child->view, NULL);
			}

			nemoview_set_parent(view, bin->parent->view);
			nemoview_set_position(view, bin->popup.x, bin->popup.y);
		} else if (bin->type == NEMOSHELL_SURFACE_OVERLAY_TYPE) {
			nemoview_set_parent(view, bin->parent->view);
			nemoview_update_transform(view);
			nemoview_damage_below(view);
		} else if (bin->type == NEMOSHELL_SURFACE_XWAYLAND_TYPE) {
			if (bin->parent == NULL) {
				nemoview_attach_layer(view, bin->layer);
				nemoview_set_position(view,
						bin->initial.x - canvas->base.width * bin->initial.dx,
						bin->initial.y - canvas->base.height * bin->initial.dy);
				nemoview_correct_pivot(view, view->content->width * bin->initial.dx, view->content->height * bin->initial.dy);
				nemoview_set_rotation(view, bin->initial.r);
				nemoview_set_scale(view, bin->initial.sx, bin->initial.sy);
				nemoview_update_transform(view);
				nemoview_damage_below(view);
			} else {
				nemoview_set_parent(view, bin->parent->view);
				nemoview_set_position(view, bin->transient.x, bin->transient.y);
				nemoview_update_transform(view);
				nemoview_damage_below(view);
			}
		}

		nemoview_transform_notify(bin->view);

		if (bin->shell->transform_bin != NULL)
			bin->shell->transform_bin(bin->shell->userdata, bin);

		bin->last_width = canvas->base.width;
		bin->last_height = canvas->base.height;

		nemoview_set_state(view, NEMOVIEW_MAP_STATE);
	} else if (bin->type == NEMOSHELL_SURFACE_OVERLAY_TYPE) {
		nemoview_update_transform_parent(view);

		bin->last_width = canvas->base.width;
		bin->last_height = canvas->base.height;
	} else if (config_changed != 0 || sx != 0 || sy != 0 ||
			bin->last_width != canvas->base.width ||
			bin->last_height != canvas->base.height ||
			bin->has_scale != 0) {
		if (nemoshell_is_nemo_surface_for_canvas(bin->canvas) != 0) {
			if ((bin->config.fullscreen || bin->config.maximized) &&
					(bin->screen.width != canvas->base.width || bin->screen.height != canvas->base.height))
				goto out;
		}

		if (nemoshell_is_nemo_surface_for_canvas(bin->canvas) != 0) {
			if (bin->has_scale != 0 && (bin->done_serial >= bin->scale.serial || bin->config.fullscreen || bin->config.maximized || (bin->scale.width == canvas->base.width && bin->scale.height == canvas->base.height))) {
				nemoview_set_scale(view, 1.0f, 1.0f);
				nemoview_update_transform(view);

				if (!wl_list_empty(&view->children_list)) {
					nemoview_update_transform_children(view);
				}

				sx = (canvas->base.width - bin->last_width) * -bin->scale.ax;
				sy = (canvas->base.height - bin->last_height) * -bin->scale.ay;

				bin->has_scale = 0;
			} else if (bin->has_scale != 0) {
				sx = (canvas->base.width - bin->last_width) * -bin->scale.ax;
				sy = (canvas->base.height - bin->last_height) * -bin->scale.ay;
			} else {
				sx = (canvas->base.width - bin->last_width) * view->geometry.ax;
				sy = (canvas->base.height - bin->last_height) * view->geometry.ay;
			}
		} else {
#define NEMOSHELL_BIN_SCALE_EPSILON			(100)

			if (bin->has_scale != 0 && (abs(bin->scale.width - canvas->base.width) < NEMOSHELL_BIN_SCALE_EPSILON || abs(bin->scale.height - canvas->base.height) < NEMOSHELL_BIN_SCALE_EPSILON)) {
				nemoview_set_scale(view, 1.0f, 1.0f);
				nemoview_update_transform(view);

				if (!wl_list_empty(&view->children_list)) {
					nemoview_update_transform_children(view);
				}

				sx = (canvas->base.width - bin->last_width) * -bin->scale.ax;
				sy = (canvas->base.height - bin->last_height) * -bin->scale.ay;

				bin->has_scale = 0;
				bin->resize_edges = 0;
			} else if (bin->resize_edges != 0) {
				sx = 0;
				sy = 0;

				if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_LEFT)
					sx = bin->last_width - canvas->base.width;
				if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_TOP)
					sy = bin->last_height - canvas->base.height;
			}
		}

		if (bin->config.fullscreen || bin->config.maximized) {
			viewanimation_revoke(view->compz, view, NEMOVIEW_TRANSLATE_ANIMATION | NEMOVIEW_ROTATE_ANIMATION);
			vieweffect_revoke(view->compz, view);

			nemoview_attach_layer(view, &bin->shell->fullscreen_layer);
			nemoview_correct_pivot(view, bin->screen.width / 2.0f, bin->screen.height / 2.0f);
			nemoview_set_position(view, bin->screen.x, bin->screen.y);
			nemoview_set_rotation(view, bin->screen.r);
		} else {
			nemoview_attach_layer(view, bin->layer);

			if (sx != 0 || sy != 0) {
				float fromx, fromy, tox, toy;

				nemoview_transform_to_global(view, 0.0f, 0.0f, &fromx, &fromy);
				nemoview_transform_to_global(view, sx, sy, &tox, &toy);

				nemoview_set_position(view,
						view->geometry.x + tox - fromx,
						view->geometry.y + toy - fromy);

				bin->reset_move = 1;

				if (nemoview_has_grab(bin->view) != 0) {
					bin->last_sx = sx;
					bin->last_sy = sy;

					wl_signal_emit(&bin->change_signal, bin);
				}
			}
		}

		if (!wl_list_empty(&view->children_list)) {
			nemoview_update_transform(view);
			nemoview_update_transform_children(view);
		}

		nemoview_transform_notify(bin->view);

		if (bin->shell->transform_bin != NULL)
			bin->shell->transform_bin(bin->shell->userdata, bin);

		bin->reset_pivot = 1;

out:
		bin->last_width = canvas->base.width;
		bin->last_height = canvas->base.height;

		wl_signal_emit(&bin->resize_signal, bin);
	}
}

static void shellbin_update_canvas_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	bin->callback->send_transform(canvas, visible, x, y, width, height);
}

static void shellbin_update_canvas_layer(struct nemocanvas *canvas, int visible)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	bin->callback->send_layer(canvas, visible);
}

static void shellbin_update_canvas_fullscreen(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	bin->callback->send_fullscreen(canvas, id, x, y, width, height);
}

static void shellbin_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct shellbin *bin = (struct shellbin *)container_of(listener, struct shellbin, canvas_destroy_listener);

	if (bin->resource != NULL)
		wl_resource_destroy(bin->resource);

	nemoshell_destroy_bin(bin);
}

struct shellbin *nemoshell_create_bin(struct nemoshell *shell, struct nemocanvas *canvas, struct nemocanvas_callback *callback)
{
	struct shellbin *bin;

	if (canvas->configure != NULL) {
		nemolog_warning("SHELL", "canvas is already owned\n");
		return NULL;
	}

	bin = (struct shellbin *)malloc(sizeof(struct shellbin));
	if (bin == NULL) {
		nemolog_error("SHELL", "failed to allocate shell bin\n");
		return NULL;
	}
	memset(bin, 0, sizeof(struct shellbin));

	bin->view = nemoview_create(canvas->compz, &canvas->base);
	if (bin->view == NULL) {
		nemolog_error("SHELL", "failed to create nemoview\n");
		goto err1;
	}

	bin->view->canvas = canvas;
	bin->view->client = wl_resource_get_client(canvas->resource);

	nemoview_set_state(bin->view, NEMOVIEW_CANVAS_STATE);

	wl_list_insert(&canvas->view_list, &bin->view->link);

	wl_signal_init(&bin->destroy_signal);
	wl_signal_init(&bin->ungrab_signal);
	wl_signal_init(&bin->change_signal);
	wl_signal_init(&bin->resize_signal);

	wl_list_init(&bin->children_list);
	wl_list_init(&bin->children_link);

	wl_list_init(&bin->screen_link);

	bin->shell = shell;
	bin->canvas = canvas;
	bin->callback = callback;
	bin->flags = NEMOSHELL_SURFACE_ALL_FLAGS;
	bin->layer = &shell->service_layer;

	bin->min_width = shell->bin.min_width;
	bin->min_height = shell->bin.min_height;
	bin->max_width = shell->bin.max_width != 0 ? shell->bin.max_width : nemocompz_get_scene_width(shell->compz);
	bin->max_height = shell->bin.max_height != 0 ? shell->bin.max_height : nemocompz_get_scene_height(shell->compz);

	bin->scale.ax = 0.5f;
	bin->scale.ay = 0.5f;

	bin->initial.sx = 1.0f;
	bin->initial.sy = 1.0f;
	bin->initial.dx = 0.5f;
	bin->initial.dy = 0.5f;

	canvas->configure = shellbin_configure_canvas;
	canvas->configure_private = (void *)bin;

	canvas->update_transform = shellbin_update_canvas_transform;
	canvas->update_layer = shellbin_update_canvas_layer;
	canvas->update_fullscreen = shellbin_update_canvas_fullscreen;

	bin->canvas_destroy_listener.notify = shellbin_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &bin->canvas_destroy_listener);

	wl_list_insert(&shell->bin_list, &bin->link);

	return bin;

err1:
	free(bin);

	return NULL;
}

void nemoshell_destroy_bin(struct shellbin *bin)
{
	struct shellbin *child, *cnext;

	wl_signal_emit(&bin->destroy_signal, bin);

	wl_list_remove(&bin->link);

	wl_list_remove(&bin->canvas_destroy_listener.link);

	bin->canvas->configure = NULL;
	bin->canvas->configure_private = NULL;

	nemoview_destroy(bin->view);

	wl_list_remove(&bin->children_link);
	wl_list_remove(&bin->screen_link);

	wl_list_for_each_safe(child, cnext, &bin->children_list, children_link) {
		nemoshell_set_parent_bin(child, NULL);
	}

	if (bin->name != NULL)
		free(bin->name);

	free(bin);
}

struct shellbin *nemoshell_get_bin(struct nemocanvas *canvas)
{
	if (canvas->configure == shellbin_configure_canvas)
		return canvas->configure_private;

	return NULL;
}

void nemoshell_set_parent_bin(struct shellbin *bin, struct shellbin *parent)
{
	bin->parent = parent;

	wl_list_remove(&bin->children_link);
	wl_list_init(&bin->children_link);

	if (parent != NULL) {
		wl_list_insert(&parent->children_list, &bin->children_link);
	}
}

struct shellbin *nemoshell_get_bin_by_pid(struct nemoshell *shell, uint32_t pid)
{
	struct shellbin *bin;

	wl_list_for_each(bin, &shell->bin_list, link) {
		if (bin->pid == pid)
			return bin;
	}

	return NULL;
}

struct shellbin *nemoshell_get_bin_by_name(struct nemoshell *shell, const char *name)
{
	struct shellbin *bin;

	wl_list_for_each(bin, &shell->bin_list, link) {
		if (bin->name != NULL && strcmp(bin->name, name) == 0)
			return bin;
	}

	return NULL;
}

static void nemoshell_bind_wayland_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	waylandshell_bind(client, data, version, id);
}

static void nemoshell_bind_xdg_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	xdgshell_bind(client, data, version, id);
}

static void nemoshell_bind_nemo_shell(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemoshell_bind(client, data, version, id);
}

static void nemoshell_handle_pointer_focus(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, pointer_focus_listener);
	struct nemopointer *pointer = (struct nemopointer *)data;

	if (pointer->focus == NULL ||
			pointer->focus->canvas == NULL)
		return;

	nemoshell_ping(
			nemoshell_get_bin(pointer->focus->canvas),
			wl_display_next_serial(shell->compz->display));
}

static void nemoshell_handle_keyboard_focus(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, keyboard_focus_listener);
	struct nemokeyboard *keyboard = (struct nemokeyboard *)data;

	if (keyboard->focused != NULL && keyboard->focused->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keyboard->focused->canvas);

		if (bin)
			nemoshell_send_xdg_config(bin);
	}

	if (keyboard->focus != NULL && keyboard->focus->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keyboard->focus->canvas);

		if (bin)
			nemoshell_send_xdg_config(bin);
	}
}

static void nemoshell_handle_keypad_focus(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, keypad_focus_listener);
	struct nemokeypad *keypad = (struct nemokeypad *)data;

	if (keypad->focused != NULL && keypad->focused->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keypad->focused->canvas);

		if (bin)
			nemoshell_send_xdg_config(bin);
	}

	if (keypad->focus != NULL && keypad->focus->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keypad->focus->canvas);

		if (bin)
			nemoshell_send_xdg_config(bin);
	}
}

static void nemoshell_handle_touch_focus(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, touch_focus_listener);
}

static void nemoshell_handle_child_signal(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, child_signal_listener);
	struct nemoproc *proc = (struct nemoproc *)data;
	struct clientstate *state;

	state = nemoshell_get_client_state(shell, proc->pid);
	if (state != NULL)
		nemoshell_destroy_client_state(shell, state);

	if (shell->destroy_client != NULL)
		shell->destroy_client(shell->userdata, proc->pid);
}

static void nemoshell_handle_idle(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, idle_listener);

	if (shell->enter_idle != NULL)
		shell->enter_idle(shell->userdata);
}

static int nemoshell_dispatch_frame_timeout(void *data)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct nemocompz *compz = shell->compz;
	struct nemoscreen *screen;
	struct nemocanvas *canvas;
	struct shellbin *bin;

	nemolog_message("SYSTEM", "cpu(%f), memory(%f)\n",
			sys_get_cpu_usage(),
			sys_get_memory_usage());

	wl_list_for_each(screen, &compz->screen_list, link) {
		nemolog_message("SYSTEM", "  SCREEN[%d:%d] frames(%d)\n", screen->node->nodeid, screen->screenid, screen->frame_count);

		screen->frame_count = 0;
	}

	wl_list_for_each(canvas, &compz->canvas_list, link) {
		bin = nemoshell_get_bin(canvas);
		if (bin != NULL) {
			nemolog_message("SYSTEM", "  CANVAS[%s:%d] frames(%d) damages(%d/%f)\n",
					bin->name == NULL ? "noname" : bin->name,
					bin->pid,
					canvas->frame_count,
					canvas->frame_damage,
					canvas->frame_count > 0 ? (double)canvas->frame_damage / 1024.0f / 1024.0f / canvas->frame_count : 0.0f);

			canvas->frame_count = 0;
			canvas->frame_damage = 0;
		}
	}

	wl_event_source_timer_update(shell->frame_timer, shell->frame_timeout);

	return 1;
}

struct nemoshell *nemoshell_create(struct nemocompz *compz)
{
	struct nemoshell *shell;

	shell = (struct nemoshell *)malloc(sizeof(struct nemoshell));
	if (shell == NULL)
		return NULL;
	memset(shell, 0, sizeof(struct nemoshell));

	shell->compz = compz;

	nemolayer_prepare(&shell->overlay_layer, &compz->cursor_layer.link);
	nemolayer_prepare(&shell->fullscreen_layer, &shell->overlay_layer.link);
	nemolayer_prepare(&shell->service_layer, &shell->fullscreen_layer.link);
	nemolayer_prepare(&shell->underlay_layer, &shell->service_layer.link);
	nemolayer_prepare(&shell->background_layer, &shell->underlay_layer.link);

#ifdef NEMOUX_WITH_WAYLANDSHELL
	if (!wl_global_create(compz->display, &wl_shell_interface, 1, shell, nemoshell_bind_wayland_shell))
		goto err1;
#endif
#ifdef NEMOUX_WITH_XDGSHELL
	if (!wl_global_create(compz->display, &xdg_shell_interface, 1, shell, nemoshell_bind_xdg_shell))
		goto err1;
#endif
	if (!wl_global_create(compz->display, &nemo_shell_interface, 1, shell, nemoshell_bind_nemo_shell))
		goto err1;

	wl_list_init(&shell->pointer_focus_listener.link);
	shell->pointer_focus_listener.notify = nemoshell_handle_pointer_focus;
	wl_signal_add(&compz->seat->pointer.focus_signal, &shell->pointer_focus_listener);

	wl_list_init(&shell->keyboard_focus_listener.link);
	shell->keyboard_focus_listener.notify = nemoshell_handle_keyboard_focus;
	wl_signal_add(&compz->seat->keyboard.focus_signal, &shell->keyboard_focus_listener);

	wl_list_init(&shell->keypad_focus_listener.link);
	shell->keypad_focus_listener.notify = nemoshell_handle_keypad_focus;
	wl_signal_add(&compz->seat->keypad.focus_signal, &shell->keypad_focus_listener);

	wl_list_init(&shell->touch_focus_listener.link);
	shell->touch_focus_listener.notify = nemoshell_handle_touch_focus;
	wl_signal_add(&compz->seat->touch.focus_signal, &shell->touch_focus_listener);

	wl_list_init(&shell->child_signal_listener.link);
	shell->child_signal_listener.notify = nemoshell_handle_child_signal;
	wl_signal_add(&compz->child_signal, &shell->child_signal_listener);

	wl_list_init(&shell->idle_listener.link);
	shell->idle_listener.notify = nemoshell_handle_idle;
	wl_signal_add(&compz->idle_signal, &shell->idle_listener);

	shell->frame_timer = wl_event_loop_add_timer(compz->loop, nemoshell_dispatch_frame_timeout, shell);
	if (shell->frame_timer == NULL)
		goto err1;

	wl_list_init(&shell->bin_list);
	wl_list_init(&shell->fullscreen_list);
	wl_list_init(&shell->stage_list);
	wl_list_init(&shell->clientstate_list);

	shell->pitch.samples = 7;
	shell->pitch.max_duration = 150;
	shell->pitch.friction = 0.015f;
	shell->pitch.coefficient = 1.8f;

	shell->pick.flags = NEMOSHELL_PICK_ALL_FLAGS;
	shell->pick.rotate_distance = 25.0f;
	shell->pick.scale_distance = 25.0f;
	shell->pick.fullscreen_scale = 1.25f;
	shell->pick.resize_interval = 50.0f;

	shell->bin.min_width = 0;
	shell->bin.min_height = 0;
	shell->bin.max_width = 0;
	shell->bin.max_height = 0;

	return shell;

err1:
	free(shell);

	return NULL;
}

void nemoshell_destroy(struct nemoshell *shell)
{
	if (shell->frame_timer != NULL)
		wl_event_source_remove(shell->frame_timer);

	nemolayer_finish(&shell->overlay_layer);
	nemolayer_finish(&shell->fullscreen_layer);
	nemolayer_finish(&shell->service_layer);
	nemolayer_finish(&shell->underlay_layer);
	nemolayer_finish(&shell->background_layer);

	free(shell);
}

void nemoshell_set_frame_timeout(struct nemoshell *shell, uint32_t timeout)
{
	shell->frame_timeout = timeout;

	wl_event_source_timer_update(shell->frame_timer, shell->frame_timeout);
}

void nemoshell_set_default_layer(struct nemoshell *shell, struct nemolayer *layer)
{
	shell->default_layer = layer;
}

void nemoshell_send_bin_close(struct shellbin *bin)
{
	bin->callback->send_close(bin->canvas);
}

void nemoshell_send_bin_config(struct shellbin *bin)
{
	struct binconfig *config;
	int32_t width, height;

	if (bin == NULL)
		return;

	if (bin->config_requested)
		config = &bin->requested_config;
	else if (bin->config_changed)
		config = &bin->next_config;
	else
		config = &bin->config;

	if (config->fullscreen || config->maximized) {
		width = bin->screen.width;
		height = bin->screen.height;
	} else {
		width = 0;
		height = 0;
	}

	bin->callback->send_configure(bin->canvas, width, height);
}

void nemoshell_change_bin_config(struct shellbin *bin)
{
	struct nemoview *parent = NULL;
	struct nemoview *view = bin->view;

	if (bin->parent != NULL && bin->parent->view != NULL)
		parent = bin->parent->view;

	if (bin->config.fullscreen || bin->config.maximized) {
		view->geometry.x = bin->fullscreen.x;
		view->geometry.y = bin->fullscreen.y;
		view->geometry.width = bin->fullscreen.width;
		view->geometry.height = bin->fullscreen.height;
		view->geometry.px = bin->fullscreen.px;
		view->geometry.py = bin->fullscreen.py;

		if (view->geometry.r != bin->fullscreen.r)
			nemoview_set_rotation(view, bin->fullscreen.r);
	}

	bin->config = bin->next_config;
	bin->config_changed = 0;

	switch (bin->type) {
		case NEMOSHELL_SURFACE_NORMAL_TYPE:
			if (bin->config.fullscreen || bin->config.maximized) {
				bin->fullscreen.x = view->geometry.x;
				bin->fullscreen.y = view->geometry.y;
				bin->fullscreen.width = view->geometry.width;
				bin->fullscreen.height = view->geometry.height;
				bin->fullscreen.r = view->geometry.r;
				bin->fullscreen.px = view->geometry.px;
				bin->fullscreen.py = view->geometry.py;
			} else if (bin->config.relative && parent) {
				nemoview_set_position(view,
						parent->geometry.x + bin->transient.x,
						parent->geometry.y + bin->transient.y);
			}
			break;

		case NEMOSHELL_SURFACE_POPUP_TYPE:
			break;

		case NEMOSHELL_SURFACE_XWAYLAND_TYPE:
			nemoview_set_position(view, bin->transient.x, bin->transient.y);
			break;

		default:
			break;
	}
}

void nemoshell_clear_bin_config(struct shellbin *bin)
{
	bin->next_config.maximized = 0;
	bin->next_config.fullscreen = 0;

	if ((bin->next_config.maximized != bin->config.maximized) ||
			(bin->next_config.fullscreen != bin->config.fullscreen))
		bin->config_changed = 1;
}

void nemoshell_send_xdg_config(struct shellbin *bin)
{
	if (xdgshell_is_xdg_surface(bin))
		nemoshell_send_bin_config(bin);
}

struct nemoview *nemoshell_get_default_view(struct nemocanvas *canvas)
{
	struct shellbin *bin;
	struct nemoview *view;

	if (canvas == NULL || wl_list_empty(&canvas->view_list))
		return NULL;

	bin = nemoshell_get_bin(canvas);
	if (bin != NULL)
		return bin->view;

	wl_list_for_each(view, &canvas->view_list, link) {
		if (nemoview_has_state(view, NEMOVIEW_MAP_STATE))
			return view;
	}

	return container_of(canvas->view_list.next, struct nemoview, link);
}

struct clientstate *nemoshell_create_client_state(struct nemoshell *shell, uint32_t pid)
{
	struct clientstate *state;

	wl_list_for_each(state, &shell->clientstate_list, link) {
		if (state->pid == pid)
			return state;
	}

	state = (struct clientstate *)malloc(sizeof(struct clientstate));
	if (state == NULL)
		return NULL;
	memset(state, 0, sizeof(struct clientstate));

	state->pid = pid;

	state->sx = 1.0f;
	state->sy = 1.0f;

	wl_list_insert(&shell->clientstate_list, &state->link);

	return state;
}

void nemoshell_destroy_client_state(struct nemoshell *shell, struct clientstate *state)
{
	wl_list_remove(&state->link);

	if (state->screenid != NULL)
		free(state->screenid);

	if (state->name != NULL)
		free(state->name);

	free(state);
}

struct clientstate *nemoshell_get_client_state(struct nemoshell *shell, uint32_t pid)
{
	struct clientstate *state;

	wl_list_for_each(state, &shell->clientstate_list, link) {
		if (state->pid == pid)
			return state;
	}

	return NULL;
}

static inline void nemoshell_set_client_state(struct shellbin *bin, struct clientstate *state)
{
	if (state->screenid != NULL) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(bin->shell, state->screenid);
		if (screen != NULL)
			nemoshell_set_fullscreen_bin(bin->shell, bin, screen);
	} else {
		if (state->has_position != 0) {
			bin->initial.x = state->x;
			bin->initial.y = state->y;
			bin->initial.has_position = 1;
		}

		bin->initial.r = state->r;
		bin->initial.sx = state->sx;
		bin->initial.sy = state->sy;
		bin->initial.dx = state->dx;
		bin->initial.dy = state->dy;
	}

	if (state->has_pickscreen != 0)
		nemoshell_bin_set_state(bin, NEMOSHELL_BIN_PICKSCREEN_STATE);
	if (state->has_pitchscreen != 0)
		nemoshell_bin_set_state(bin, NEMOSHELL_BIN_PITCHSCREEN_STATE);

	bin->flags = state->flags;

	if (state->name != NULL)
		bin->name = strdup(state->name);

	nemoview_set_state(bin->view, state->state_on);
	nemoview_put_state(bin->view, state->state_off);
}

int nemoshell_use_client_state(struct nemoshell *shell, struct shellbin *bin)
{
	struct clientstate *state;
	pixman_box32_t *extents;
	pid_t pid = bin->pid;

	state = nemoshell_get_client_state(shell, pid);
	if (state != NULL) {
		nemoshell_set_client_state(bin, state);

		return 1;
	} else {
		pid_t ppid;

		if (sys_get_process_parent_id(pid, &ppid) > 0) {
			state = nemoshell_get_client_state(shell, ppid);
			if (state != NULL) {
				bin->pid = ppid;

				nemoshell_set_client_state(bin, state);

				return 1;
			}
		}
	}

	return 0;
}

int nemoshell_use_client_stage(struct nemoshell *shell, struct shellbin *bin)
{
	struct shellstage *stage;

	stage = nemoshell_get_stage_on(shell, bin->initial.x, bin->initial.y);
	if (stage != NULL) {
		bin->initial.r = stage->dr * M_PI / 180.0f;

		return 1;
	}

	return 0;
}

struct shellscreen *nemoshell_get_fullscreen(struct nemoshell *shell, const char *id)
{
	struct shellscreen *screen;

	wl_list_for_each(screen, &shell->fullscreen_list, link) {
		if (strcmp(screen->id, id) == 0)
			return screen;
	}

	screen = (struct shellscreen *)malloc(sizeof(struct shellscreen));
	if (screen == NULL)
		return NULL;
	memset(screen, 0, sizeof(struct shellscreen));

	screen->id = strdup(id);

	wl_list_init(&screen->bin_list);

	wl_list_insert(&shell->fullscreen_list, &screen->link);

	return screen;
}

void nemoshell_put_fullscreen(struct nemoshell *shell, const char *id)
{
	struct shellscreen *screen;

	wl_list_for_each(screen, &shell->fullscreen_list, link) {
		if (strcmp(screen->id, id) == 0) {
			wl_list_remove(&screen->link);
			wl_list_remove(&screen->bin_list);

			free(screen->id);
			free(screen);

			break;
		}
	}
}

struct shellscreen *nemoshell_get_fullscreen_on(struct nemoshell *shell, int32_t x, int32_t y, uint32_t type)
{
	struct shellscreen *screen;

	wl_list_for_each(screen, &shell->fullscreen_list, link) {
		if (screen->type != type)
			continue;

		if (screen->sx <= x && x <= screen->sx + screen->sw &&
				screen->sy <= y && y <= screen->sy + screen->sh)
			return screen;
	}

	return NULL;
}

struct shellstage *nemoshell_get_stage(struct nemoshell *shell, const char *id)
{
	struct shellstage *stage;

	wl_list_for_each(stage, &shell->stage_list, link) {
		if (strcmp(stage->id, id) == 0)
			return stage;
	}

	stage = (struct shellstage *)malloc(sizeof(struct shellstage));
	if (stage == NULL)
		return NULL;
	memset(stage, 0, sizeof(struct shellstage));

	stage->id = strdup(id);

	wl_list_insert(&shell->stage_list, &stage->link);

	return stage;
}

void nemoshell_put_stage(struct nemoshell *shell, const char *id)
{
	struct shellstage *stage;

	wl_list_for_each(stage, &shell->stage_list, link) {
		if (strcmp(stage->id, id) == 0) {
			wl_list_remove(&stage->link);

			free(stage->id);
			free(stage);

			break;
		}
	}
}

struct shellstage *nemoshell_get_stage_on(struct nemoshell *shell, int32_t x, int32_t y)
{
	struct shellstage *stage;

	wl_list_for_each(stage, &shell->stage_list, link) {
		if (stage->sx <= x && x <= stage->sx + stage->sw &&
				stage->sy <= y && y <= stage->sy + stage->sh)
			return stage;
	}

	return NULL;
}

void nemoshell_set_fullscreen_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen)
{
	nemoshell_clear_bin_config(bin);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->requested_config.fullscreen = 1;
		bin->config_requested = 1;
	} else {
		bin->next_config.fullscreen = 1;
		bin->config_changed = 1;
	}

	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->rx;
	bin->screen.y = screen->ry;
	bin->screen.width = screen->rw;
	bin->screen.height = screen->rh;
	bin->has_screen = 1;

	nemoshell_bin_set_state(bin, NEMOSHELL_BIN_FIXED_STATE);

	nemoshell_send_bin_config(bin);
}

void nemoshell_set_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen)
{
	wl_list_insert(&screen->bin_list, &bin->screen_link);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->requested_config.fullscreen = 1;
		bin->config_requested = 1;
	} else {
		nemoshell_clear_bin_config(bin);
		bin->next_config.fullscreen = 1;
		bin->config_changed = 1;
	}

	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->dx;
	bin->screen.y = screen->dy;
	bin->screen.width = screen->dw;
	bin->screen.height = screen->dh;
	bin->screen.r = screen->dr * M_PI / 180.0f;
	bin->has_screen = 1;

	if (screen->fixed != 0)
		nemoshell_bin_set_state(bin, NEMOSHELL_BIN_FIXED_STATE);
	else
		nemoshell_bin_put_state(bin, NEMOSHELL_BIN_FIXED_STATE);

	nemocontent_update_fullscreen(bin->view->content, screen->id, screen->dx, screen->dy, screen->dw, screen->dh);

	nemoshell_send_bin_config(bin);
}

void nemoshell_put_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin)
{
	wl_list_remove(&bin->screen_link);
	wl_list_init(&bin->screen_link);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->config_requested = 1;
		bin->requested_config.fullscreen = 0;
	} else {
		bin->config_changed = 1;
		bin->next_config.fullscreen = 0;
	}

	nemoshell_bin_put_state(bin, NEMOSHELL_BIN_FIXED_STATE);

	nemoshell_send_bin_config(bin);

	nemocontent_update_fullscreen(bin->view->content, NULL, bin->screen.x, bin->screen.y, bin->screen.width, bin->screen.height);
}

static inline int nemoshell_bin_contain_view(struct shellbin *bin, struct nemoview *view)
{
	float s[4][2] = {
		{ 0.0f, 0.0f },
		{ view->content->width, 0.0f },
		{ 0.0f, view->content->height },
		{ view->content->width, view->content->height }
	};
	float x, y;
	int i;

	for (i = 0; i < 4; i++) {
		nemoview_transform_to_global(view, s[i][0], s[i][1], &x, &y);

		if (x < bin->screen.x || x > bin->screen.x + bin->screen.width ||
				y < bin->screen.y || y > bin->screen.y + bin->screen.height)
			return 0;
	}

	return 1;
}

void nemoshell_set_maximized_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen)
{
	nemoshell_clear_bin_config(bin);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->requested_config.maximized = 1;
		bin->config_requested = 1;
	} else {
		bin->next_config.maximized = 1;
		bin->config_changed = 1;
	}

	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->rx;
	bin->screen.y = screen->ry;
	bin->screen.width = screen->rw;
	bin->screen.height = screen->rh;
	bin->has_screen = 1;

	nemoshell_bin_set_state(bin, NEMOSHELL_BIN_FIXED_STATE);

	nemoshell_send_bin_config(bin);
}

void nemoshell_set_maximized_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen)
{
	wl_list_insert(&screen->bin_list, &bin->screen_link);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->requested_config.maximized = 1;
		bin->config_requested = 1;
	} else {
		nemoshell_clear_bin_config(bin);
		bin->next_config.maximized = 1;
		bin->config_changed = 1;
	}

	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->dx;
	bin->screen.y = screen->dy;
	bin->screen.width = screen->dw;
	bin->screen.height = screen->dh;
	bin->screen.r = screen->dr * M_PI / 180.0f;
	bin->has_screen = 1;

	if (screen->fixed != 0)
		nemoshell_bin_set_state(bin, NEMOSHELL_BIN_FIXED_STATE);
	else
		nemoshell_bin_put_state(bin, NEMOSHELL_BIN_FIXED_STATE);

	nemoshell_send_bin_config(bin);
}

void nemoshell_put_maximized_bin(struct nemoshell *shell, struct shellbin *bin)
{
	wl_list_remove(&bin->screen_link);
	wl_list_init(&bin->screen_link);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->config_requested = 1;
		bin->requested_config.maximized = 0;
	} else {
		bin->config_changed = 1;
		bin->next_config.maximized = 0;
	}

	nemoshell_bin_put_state(bin, NEMOSHELL_BIN_FIXED_STATE);

	nemoshell_send_bin_config(bin);
}
