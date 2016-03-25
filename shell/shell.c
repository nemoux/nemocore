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
#include <prochelper.h>
#include <nemomisc.h>
#include <nemolog.h>
#include <nemoitem.h>

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
	int state_changed = 0;

	assert(bin);

	if (canvas->base.width == 0)
		return;

	if (bin->has_set_geometry == 0 && bin->has_next_geometry != 0) {
		bin->geometry = bin->next_geometry;
		bin->has_next_geometry = 0;
		bin->has_set_geometry = 1;
	}

	if (bin->state_changed != 0) {
		if (xdgshell_is_xdg_surface(bin) || waylandshell_is_shell_surface(bin)) {
			nemoshell_change_bin_next_state(bin);
		} else {
			bin->state = bin->next_state;
			bin->state_changed = 0;
		}

		state_changed = 1;
	}

	if (!nemoview_has_state(view, NEMO_VIEW_MAP_STATE)) {
		if (bin->type == NEMO_SHELL_SURFACE_NORMAL_TYPE) {
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
							bin->geometry.x + canvas->base.width * view->geometry.ax,
							bin->geometry.y + canvas->base.height * view->geometry.ay);

					if (bin->view->geometry.has_pivot == 0)
						nemoview_correct_pivot(bin->view, view->content->width * -view->geometry.ax, view->content->height * -view->geometry.ay);
				} else {
					nemoview_set_position(view,
							bin->geometry.x - canvas->base.width * bin->geometry.dx,
							bin->geometry.y - canvas->base.height * bin->geometry.dy);

					if (bin->view->geometry.has_pivot == 0)
						nemoview_correct_pivot(bin->view, view->content->width * bin->geometry.dx, view->content->height * bin->geometry.dy);
				}
				nemoview_set_rotation(view, bin->geometry.r);

				nemoview_attach_layer(view, bin->layer);
				nemoview_update_transform(view);
				nemoview_damage_below(view);
			}
		} else if (bin->type == NEMO_SHELL_SURFACE_POPUP_TYPE) {
			struct shellbin *child;

			nemoview_update_transform(view);
			nemoview_damage_below(view);

			wl_list_for_each_reverse(child, &bin->children_list, children_link) {
				nemoview_above_layer(child->view, NULL);
			}

			nemoview_set_parent(view, bin->parent->view);
			nemoview_set_position(view, bin->popup.x, bin->popup.y);
		} else if (bin->type == NEMO_SHELL_SURFACE_OVERLAY_TYPE) {
			nemoview_set_parent(view, bin->parent->view);
			nemoview_update_transform(view);
			nemoview_damage_below(view);
		} else if (bin->type == NEMO_SHELL_SURFACE_XWAYLAND_TYPE) {
			nemoview_attach_layer(view, bin->layer);
			nemoview_set_position(view,
					bin->geometry.x - canvas->base.width * bin->geometry.dx,
					bin->geometry.y - canvas->base.height * bin->geometry.dy);
			nemoview_correct_pivot(view, view->content->width * bin->geometry.dx, view->content->height * bin->geometry.dy);
			nemoview_set_rotation(view, bin->geometry.r);
			nemoview_update_transform(view);
			nemoview_damage_below(view);
		}

		nemoview_transform_done(bin->view);

		bin->last_width = canvas->base.width;
		bin->last_height = canvas->base.height;

		nemoview_set_state(view, NEMO_VIEW_MAP_STATE);
	} else if (bin->type == NEMO_SHELL_SURFACE_OVERLAY_TYPE) {
		nemoview_update_transform_parent(view);

		bin->last_width = canvas->base.width;
		bin->last_height = canvas->base.height;
	} else if (state_changed != 0 || sx != 0 || sy != 0 ||
			bin->last_width != canvas->base.width || bin->last_height != canvas->base.height) {
		if (nemoshell_is_nemo_surface_for_canvas(bin->canvas) != 0) {
			if ((bin->state.fullscreen || bin->state.maximized) &&
					(bin->screen.width != canvas->base.width || bin->screen.height != canvas->base.height))
				goto out;
		}

		if (nemoshell_is_nemo_surface_for_canvas(bin->canvas) != 0) {
			if (bin->reset_scale != 0) {
				nemoview_set_scale(view, 1.0f, 1.0f);
				nemoview_update_transform(view);

				if (!wl_list_empty(&view->children_list)) {
					nemoview_update_transform_children(view);
				}

				sx = (canvas->base.width - bin->last_width) * -bin->ax;
				sy = (canvas->base.height - bin->last_height) * -bin->ay;

				bin->reset_scale = 0;
			} else {
				sx = (canvas->base.width - bin->last_width) * view->geometry.ax;
				sy = (canvas->base.height - bin->last_height) * view->geometry.ay;
			}
		} else {
			if (bin->resize_edges != 0) {
				sx = 0;
				sy = 0;
			}

			if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_LEFT) {
				sx = bin->last_width - canvas->base.width;
			}
			if (bin->resize_edges & WL_SHELL_SURFACE_RESIZE_TOP) {
				sy = bin->last_height - canvas->base.height;
			}
		}

		if (bin->state.fullscreen || bin->state.maximized) {
			viewanimation_revoke(view->compz, view);
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

				if (bin->grabbed > 0) {
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

		nemoview_transform_done(bin->view);

out:
		bin->last_width = canvas->base.width;
		bin->last_height = canvas->base.height;

		wl_signal_emit(&bin->resize_signal, bin);
	}
}

static void shellbin_update_canvas_transform(struct nemocanvas *canvas, int visible)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	bin->client->send_transform(canvas, visible);
}

static void shellbin_update_canvas_fullscreen(struct nemocanvas *canvas, int active, int opaque)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	bin->client->send_fullscreen(canvas, active, opaque);
}

static void shellbin_update_canvas_layer(struct nemocanvas *canvas, int on_top)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	bin->client->send_layer(canvas, on_top);
}

static void shellbin_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct shellbin *bin = (struct shellbin *)container_of(listener, struct shellbin, canvas_destroy_listener);

	if (bin->resource != NULL)
		wl_resource_destroy(bin->resource);

	nemoshell_destroy_bin(bin);
}

struct shellbin *nemoshell_create_bin(struct nemoshell *shell, struct nemocanvas *canvas, struct nemoclient *client)
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
	bin->client = client;
	bin->flags = NEMO_SHELL_SURFACE_ALL_FLAGS;
	bin->layer = &shell->service_layer;

	bin->min_width = shell->bin.min_width;
	bin->min_height = shell->bin.min_height;
	bin->max_width = shell->bin.max_width;
	bin->max_height = shell->bin.max_height;

	bin->ax = 0.5f;
	bin->ay = 0.5f;

	canvas->configure = shellbin_configure_canvas;
	canvas->configure_private = (void *)bin;

	canvas->update_transform = shellbin_update_canvas_transform;
	canvas->update_fullscreen = shellbin_update_canvas_fullscreen;
	canvas->update_layer = shellbin_update_canvas_layer;

	bin->canvas_destroy_listener.notify = shellbin_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &bin->canvas_destroy_listener);

	wl_list_init(&bin->fullscreen_opaque_listener.link);

	return bin;

err1:
	free(bin);

	return NULL;
}

void nemoshell_destroy_bin(struct shellbin *bin)
{
	struct shellbin *child, *cnext;

	wl_signal_emit(&bin->destroy_signal, bin);

	wl_list_remove(&bin->canvas_destroy_listener.link);
	wl_list_remove(&bin->fullscreen_opaque_listener.link);

	bin->canvas->configure = NULL;
	bin->canvas->configure_private = NULL;

	nemoview_destroy(bin->view);

	wl_list_remove(&bin->children_link);
	wl_list_remove(&bin->screen_link);

	wl_list_for_each_safe(child, cnext, &bin->children_list, children_link) {
		nemoshell_set_parent_bin(child, NULL);
	}

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

		if (bin) {
			nemoshell_send_bin_configure(bin);
		}
	}

	if (keyboard->focus != NULL && keyboard->focus->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keyboard->focus->canvas);

		if (bin) {
			nemoshell_send_bin_configure(bin);
		}
	}
}

static void nemoshell_handle_keypad_focus(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, keypad_focus_listener);
	struct nemokeypad *keypad = (struct nemokeypad *)data;

	if (keypad->focused != NULL && keypad->focused->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keypad->focused->canvas);

		if (bin) {
			nemoshell_send_bin_configure(bin);
		}
	}

	if (keypad->focus != NULL && keypad->focus->canvas != NULL) {
		struct shellbin *bin = nemoshell_get_bin(keypad->focus->canvas);

		if (bin) {
			nemoshell_send_bin_configure(bin);
		}
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
}

struct nemoshell *nemoshell_create(struct nemocompz *compz)
{
	struct nemoshell *shell;
	char *env;

	shell = (struct nemoshell *)malloc(sizeof(struct nemoshell));
	if (shell == NULL)
		return NULL;
	memset(shell, 0, sizeof(struct nemoshell));

	shell->compz = compz;
	shell->configs = compz->configs;

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

	wl_list_init(&shell->fullscreen_list);
	wl_list_init(&shell->clientstate_list);

	env = getenv("NEMOUX_SHELL_GRAB_LOG");
	if (env != NULL && strcmp(env, "ON") == 0)
		shell->is_logging_grab = 1;

	return shell;

err1:
	free(shell);

	return NULL;
}

void nemoshell_destroy(struct nemoshell *shell)
{
	nemolayer_finish(&shell->overlay_layer);
	nemolayer_finish(&shell->fullscreen_layer);
	nemolayer_finish(&shell->service_layer);
	nemolayer_finish(&shell->underlay_layer);
	nemolayer_finish(&shell->background_layer);

	free(shell);
}

void nemoshell_set_default_layer(struct nemoshell *shell, struct nemolayer *layer)
{
	shell->default_layer = layer;
}

void nemoshell_send_bin_state(struct shellbin *bin)
{
	struct binstate *state;
	int32_t width, height;

	if (bin == NULL)
		return;

	if (bin->state_requested)
		state = &bin->requested_state;
	else if (bin->state_changed)
		state = &bin->next_state;
	else
		state = &bin->state;

	if (state->fullscreen || state->maximized) {
		width = bin->screen.width;
		height = bin->screen.height;
	} else {
		width = 0;
		height = 0;
	}

	bin->client->send_configure(bin->canvas, width, height);
}

void nemoshell_send_bin_configure(struct shellbin *bin)
{
	if (xdgshell_is_xdg_surface(bin))
		nemoshell_send_bin_state(bin);
}

void nemoshell_change_bin_next_state(struct shellbin *bin)
{
	struct nemoview *parent = NULL;
	struct nemoview *view = bin->view;

	if (bin->parent != NULL && bin->parent->view != NULL)
		parent = bin->parent->view;

	if (bin->state.fullscreen || bin->state.maximized) {
		view->geometry.x = bin->fullscreen.x;
		view->geometry.y = bin->fullscreen.y;
		view->geometry.width = bin->fullscreen.width;
		view->geometry.height = bin->fullscreen.height;

		if (view->geometry.r != bin->fullscreen.r)
			nemoview_set_rotation(view, bin->fullscreen.r);
	}

	bin->state = bin->next_state;
	bin->state_changed = 0;

	switch (bin->type) {
		case NEMO_SHELL_SURFACE_NORMAL_TYPE:
			if (bin->state.fullscreen || bin->state.maximized) {
				bin->fullscreen.x = view->geometry.x;
				bin->fullscreen.y = view->geometry.y;
				bin->fullscreen.width = view->geometry.width;
				bin->fullscreen.height = view->geometry.height;
				bin->fullscreen.r = view->geometry.r;
			} else if (bin->state.relative && parent) {
				nemoview_set_position(view,
						parent->geometry.x + bin->transient.x,
						parent->geometry.y + bin->transient.y);
			}
			break;

		case NEMO_SHELL_SURFACE_POPUP_TYPE:
			break;

		case NEMO_SHELL_SURFACE_XWAYLAND_TYPE:
			nemoview_set_position(view, bin->transient.x, bin->transient.y);
			break;

		default:
			break;
	}
}

void nemoshell_clear_bin_next_state(struct shellbin *bin)
{
	bin->next_state.maximized = 0;
	bin->next_state.fullscreen = 0;

	if ((bin->next_state.maximized != bin->state.maximized) ||
			(bin->next_state.fullscreen != bin->state.fullscreen))
		bin->state_changed = 1;
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
		if (nemoview_has_state(view, NEMO_VIEW_MAP_STATE))
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

	wl_list_insert(&shell->clientstate_list, &state->link);

	return state;
}

void nemoshell_destroy_client_state(struct nemoshell *shell, struct clientstate *state)
{
	wl_list_remove(&state->link);

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
	if (state->is_fullscreen || state->is_maximized) {
		nemoshell_clear_bin_next_state(bin);
		bin->requested_state.maximized = state->is_maximized;
		bin->requested_state.fullscreen = state->is_fullscreen;
		bin->state_requested = 1;

		bin->screen.x = state->x;
		bin->screen.y = state->y;
		bin->screen.width = state->width;
		bin->screen.height = state->height;
		bin->has_screen = 1;

		nemoshell_send_bin_state(bin);
	} else {
		bin->geometry.x = state->x;
		bin->geometry.y = state->y;
		bin->geometry.r = state->r;
		bin->geometry.dx = state->dx;
		bin->geometry.dy = state->dy;
		bin->has_set_geometry = 1;
	}

	if (state->has_max_size != 0) {
		bin->max_width = state->max_width;
		bin->max_height = state->max_height;
	}
	if (state->has_min_size != 0) {
		bin->min_width = state->min_width;
		bin->min_height = state->min_height;
	}

	if (state->fadein_type != 0) {
		struct viewanimation *animation;

		animation = viewanimation_create(bin->view, state->fadein_ease, state->fadein_delay, state->fadein_duration);
		if (state->fadein_type & NEMO_SHELL_FADEIN_ALPHA_FLAG) {
			nemoview_set_alpha(bin->view, 0.0f);

			viewanimation_set_alpha(animation, 1.0f);
		}

		if (state->fadein_type & NEMO_SHELL_FADEIN_SCALE_FLAG) {
			nemoview_set_scale(bin->view, 0.0f, 0.0f);

			viewanimation_set_scale(animation, 1.0f, 1.0f);
		}

		viewanimation_dispatch(bin->shell->compz, animation);
	}

	bin->flags = state->flags;

	nemoview_set_state(bin->view, state->state_on);
	nemoview_put_state(bin->view, state->state_off);
}

int nemoshell_use_client_state(struct nemoshell *shell, struct shellbin *bin, struct wl_client *client)
{
	struct clientstate *state;
	pid_t pid;

	wl_client_get_credentials(client, &pid, NULL, NULL);

	state = nemoshell_get_client_state(shell, pid);
	if (state != NULL) {
		nemoshell_set_client_state(bin, state);

		return 1;
	} else {
		pid_t ppid;

		if (proc_get_process_parent_id(pid, &ppid) > 0) {
			state = nemoshell_get_client_state(shell, ppid);
			if (state != NULL) {
				nemoshell_set_client_state(bin, state);

				return 1;
			}
		}
	}

	return 0;
}

int nemoshell_use_client_state_by_pid(struct nemoshell *shell, struct shellbin *bin, pid_t pid)
{
	struct clientstate *state;

	state = nemoshell_get_client_state(shell, pid);
	if (state != NULL) {
		nemoshell_set_client_state(bin, state);

		return 1;
	} else {
		pid_t ppid;

		if (proc_get_process_parent_id(pid, &ppid) > 0) {
			state = nemoshell_get_client_state(shell, ppid);
			if (state != NULL) {
				nemoshell_set_client_state(bin, state);

				return 1;
			}
		}
	}

	return 0;
}

void nemoshell_load_fullscreens(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;
	struct shellscreen *screen;
	char *type;
	char *focus;
	char *fixed;
	int index;

	nemoitem_for_each(shell->configs, index, "//nemoshell/fullscreen", 0) {
		screen = (struct shellscreen *)malloc(sizeof(struct shellscreen));
		if (screen == NULL)
			break;
		memset(screen, 0, sizeof(struct shellscreen));

		screen->sx = nemoitem_get_iattr(shell->configs, index, "sx", 0);
		screen->sy = nemoitem_get_iattr(shell->configs, index, "sy", 0);
		screen->sw = nemoitem_get_iattr(shell->configs, index, "sw", nemocompz_get_scene_width(compz));
		screen->sh = nemoitem_get_iattr(shell->configs, index, "sh", nemocompz_get_scene_height(compz));
		screen->dx = nemoitem_get_iattr(shell->configs, index, "dx", 0);
		screen->dy = nemoitem_get_iattr(shell->configs, index, "dy", 0);
		screen->dw = nemoitem_get_iattr(shell->configs, index, "dw", nemocompz_get_scene_width(compz));
		screen->dh = nemoitem_get_iattr(shell->configs, index, "dh", nemocompz_get_scene_height(compz));
		screen->dr = nemoitem_get_iattr(shell->configs, index, "dr", 0);
		screen->id = nemoitem_get_iattr(shell->configs, index, "id", 0);

		type = nemoitem_get_attr(shell->configs, index, "type");
		if (type == NULL)
			screen->type = NEMO_SHELL_FULLSCREEN_NORMAL_TYPE;
		else if (strcmp(type, "pick") == 0)
			screen->type = NEMO_SHELL_FULLSCREEN_PICK_TYPE;
		else if (strcmp(type, "pitch") == 0)
			screen->type = NEMO_SHELL_FULLSCREEN_PITCH_TYPE;

		focus = nemoitem_get_attr(shell->configs, index, "focus");
		if (focus == NULL)
			screen->focus = NEMO_SHELL_FULLSCREEN_NONE_FOCUS;
		else if (strcmp(focus, "all") == 0)
			screen->focus = NEMO_SHELL_FULLSCREEN_ALL_FOCUS;

		fixed = nemoitem_get_attr(shell->configs, index, "fixed");
		if (fixed == NULL)
			screen->fixed = 0;
		else if (strcmp(fixed, "on") == 0)
			screen->fixed = 1;

		wl_list_init(&screen->bin_list);

		wl_list_insert(&shell->fullscreen_list, &screen->link);
	}
}

struct shellscreen *nemoshell_get_fullscreen(struct nemoshell *shell, uint32_t id)
{
	struct shellscreen *screen;

	wl_list_for_each(screen, &shell->fullscreen_list, link) {
		if (screen->id == id)
			return screen;
	}

	return NULL;
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

void nemoshell_set_toplevel_bin(struct nemoshell *shell, struct shellbin *bin)
{
	nemoshell_clear_bin_next_state(bin);

	bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	nemoshell_set_parent_bin(bin, NULL);
}

void nemoshell_set_popup_bin(struct nemoshell *shell, struct shellbin *bin, struct shellbin *parent, int32_t x, int32_t y, uint32_t serial)
{
	nemoshell_clear_bin_next_state(bin);

	bin->type = NEMO_SHELL_SURFACE_POPUP_TYPE;
	bin->popup.x = x;
	bin->popup.y = y;
	bin->popup.serial = serial;

	nemoshell_set_parent_bin(bin, parent);
}

void nemoshell_set_fullscreen_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen)
{
	nemoshell_clear_bin_next_state(bin);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->requested_state.fullscreen = 1;
		bin->state_requested = 1;
	} else {
		bin->next_state.fullscreen = 1;
		bin->state_changed = 1;
	}

	bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->rx;
	bin->screen.y = screen->ry;
	bin->screen.width = screen->rw;
	bin->screen.height = screen->rh;
	bin->has_screen = 1;

	bin->fixed = 1;

	nemoshell_send_bin_state(bin);
}

void nemoshell_set_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen)
{
	wl_list_insert(&screen->bin_list, &bin->screen_link);

	nemoshell_clear_bin_next_state(bin);
	bin->next_state.fullscreen = 1;
	bin->state_changed = 1;

	bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->dx;
	bin->screen.y = screen->dy;
	bin->screen.width = screen->dw;
	bin->screen.height = screen->dh;
	bin->screen.r = screen->dr * M_PI / 180.0f;
	bin->has_screen = 1;

	bin->fixed = screen->fixed;

	nemoshell_send_bin_state(bin);
}

void nemoshell_put_fullscreen_bin(struct nemoshell *shell, struct shellbin *bin)
{
	wl_list_remove(&bin->screen_link);
	wl_list_init(&bin->screen_link);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->state_requested = 1;
		bin->requested_state.fullscreen = 0;
	} else {
		bin->state_changed = 1;
		bin->next_state.fullscreen = 0;
	}

	bin->fixed = 0;

	nemoshell_send_bin_state(bin);
}

static inline int nemoshell_bin_contains_view(struct shellbin *bin, struct nemoview *view)
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

static void shellbin_handle_fullscreen_opaque(struct wl_listener *listener, void *data)
{
	struct shellbin *bin = (struct shellbin *)container_of(listener, struct shellbin, fullscreen_opaque_listener);

	nemoshell_put_fullscreen_opaque(bin->shell, bin);
}

void nemoshell_set_fullscreen_opaque(struct nemoshell *shell, struct shellbin *bin)
{
	struct nemocompz *compz = shell->compz;
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (nemoshell_bin_contains_view(bin, child) != 0)
						nemocontent_update_fullscreen(child->content, 1, bin->on_opaquescreen);
				}
			}

			if (nemoshell_bin_contains_view(bin, view) != 0)
				nemocontent_update_fullscreen(view->content, 1, bin->on_opaquescreen);
		}
	}

	wl_list_remove(&bin->fullscreen_opaque_listener.link);

	bin->fullscreen_opaque_listener.notify = shellbin_handle_fullscreen_opaque;
	wl_signal_add(&bin->destroy_signal, &bin->fullscreen_opaque_listener);
}

void nemoshell_put_fullscreen_opaque(struct nemoshell *shell, struct shellbin *bin)
{
	struct nemocompz *compz = shell->compz;
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (bin->view == view)
				continue;

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (nemoshell_bin_contains_view(bin, child) != 0)
						nemocontent_update_fullscreen(child->content, 0, bin->on_opaquescreen);
				}
			}

			if (nemoshell_bin_contains_view(bin, view) != 0)
				nemocontent_update_fullscreen(view->content, 0, bin->on_opaquescreen);
		}
	}

	wl_list_remove(&bin->fullscreen_opaque_listener.link);
	wl_list_init(&bin->fullscreen_opaque_listener.link);
}

void nemoshell_set_maximized_bin_on_screen(struct nemoshell *shell, struct shellbin *bin, struct nemoscreen *screen)
{
	nemoshell_clear_bin_next_state(bin);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->requested_state.maximized = 1;
		bin->state_requested = 1;
	} else {
		bin->next_state.maximized = 1;
		bin->state_changed = 1;
	}

	bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->rx;
	bin->screen.y = screen->ry;
	bin->screen.width = screen->rw;
	bin->screen.height = screen->rh;
	bin->has_screen = 1;

	bin->fixed = 1;

	nemoshell_send_bin_state(bin);
}

void nemoshell_set_maximized_bin(struct nemoshell *shell, struct shellbin *bin, struct shellscreen *screen)
{
	wl_list_insert(&screen->bin_list, &bin->screen_link);

	nemoshell_clear_bin_next_state(bin);
	bin->next_state.maximized = 1;
	bin->state_changed = 1;

	bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	nemoshell_set_parent_bin(bin, NULL);

	bin->screen.x = screen->dx;
	bin->screen.y = screen->dy;
	bin->screen.width = screen->dw;
	bin->screen.height = screen->dh;
	bin->screen.r = screen->dr * M_PI / 180.0f;
	bin->has_screen = 1;

	bin->fixed = screen->fixed;

	nemoshell_send_bin_state(bin);
}

void nemoshell_put_maximized_bin(struct nemoshell *shell, struct shellbin *bin)
{
	wl_list_remove(&bin->screen_link);
	wl_list_init(&bin->screen_link);

	if (xdgshell_is_xdg_surface(bin) || xdgshell_is_xdg_popup(bin)) {
		bin->state_requested = 1;
		bin->requested_state.maximized = 0;
	} else {
		bin->state_changed = 1;
		bin->next_state.maximized = 0;
	}

	bin->fixed = 0;

	nemoshell_send_bin_state(bin);
}

void nemoshell_load_gestures(struct nemoshell *shell)
{
	shell->pitch.samples = nemoitem_get_iattr_named(shell->configs, "//nemoshell/pitch", "samples", 7);
	shell->pitch.max_duration = nemoitem_get_iattr_named(shell->configs, "//nemoshell/pitch", "max_duration", 150);
	shell->pitch.friction = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pitch", "friction", 0.015f);
	shell->pitch.coefficient = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pitch", "coefficient", 1.8f);

	shell->pick.samples = nemoitem_get_iattr_named(shell->configs, "//nemoshell/pick", "samples", 5);
	shell->pick.min_velocity = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pick", "min_velocity", 0.1f);
	shell->pick.rotate_degree = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pick", "rotate_degree", 5.0f);
	shell->pick.scale_degree = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pick", "scale_degree", 45.0f);
	shell->pick.fullscreen_scale = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pick", "fullscreen_scale", 1.5f);
	shell->pick.resize_interval = nemoitem_get_fattr_named(shell->configs, "//nemoshell/pick", "resize_interval", 50.0f);

	shell->bin.min_width = nemoitem_get_iattr_named(shell->configs, "//nemoshell/bin", "min_width", 0);
	shell->bin.min_height = nemoitem_get_iattr_named(shell->configs, "//nemoshell/bin", "min_height", 0);
	shell->bin.max_width = nemoitem_get_iattr_named(shell->configs, "//nemoshell/bin", "max_width", nemocompz_get_scene_width(shell->compz) * 1.5f);
	shell->bin.max_height = nemoitem_get_iattr_named(shell->configs, "//nemoshell/bin", "max_height", nemocompz_get_scene_height(shell->compz) * 1.5f);
}
