#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-xdg-shell-server-protocol.h>

#include <compz.h>
#include <shell.h>
#include <xdgshell.h>
#include <canvas.h>
#include <view.h>
#include <screen.h>
#include <move.h>
#include <resize.h>
#include <nemomisc.h>

static void xdg_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);
	struct wl_array states;
	uint32_t *s;
	uint32_t serial;

	assert(bin);

	if (bin->resource == NULL)
		return;

	wl_array_init(&states);

	if (bin->requested_state.fullscreen) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_FULLSCREEN;
	} else if (bin->requested_state.maximized) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_MAXIMIZED;
	}
	if (bin->resize_edges != 0) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_RESIZING;
	}
	if (bin->view->focus.keyboard_count > 0) {
		s = wl_array_add(&states, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_ACTIVATED;
	}

	serial = wl_display_next_serial(canvas->compz->display);
	xdg_surface_send_configure(bin->resource, width, height, &states, serial);

	wl_array_release(&states);
}

static struct nemoclient xdg_client = {
	xdg_send_configure
};

static void xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void xdg_surface_set_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent_resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct shellbin *parent;

	if (parent_resource != NULL)
		parent = nemoshell_get_bin((struct nemocanvas *)wl_resource_get_user_data(parent_resource));
	else
		parent = NULL;

	nemoshell_set_parent_bin(bin, parent);
}

static void xdg_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title)
{
}

static void xdg_surface_set_app_id(struct wl_client *client, struct wl_resource *resource, const char *appid)
{
}

static void xdg_surface_show_window_menu(struct wl_client *client, struct wl_resource *surface_resource, struct wl_resource *seat_resource, uint32_t serial, int32_t x, int32_t y)
{
}

static void xdg_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_MOVABLE_FLAG) {
		nemoshell_move_canvas(bin->shell, bin, serial);
	}
}

static void xdg_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial, uint32_t edges)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_RESIZABLE_FLAG) {
		nemoshell_resize_canvas(bin->shell, bin, serial, edges);
	}
}

static void xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->state_requested) {
		bin->next_state = bin->requested_state;
		bin->state_changed = 1;
		bin->state_requested = 0;
	}
}

static void xdg_surface_set_window_geometry(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->next_geometry.x = x;
	bin->next_geometry.y = y;
	bin->next_geometry.width = width;
	bin->next_geometry.height = height;
	bin->has_next_geometry = 1;
}

static void xdg_surface_set_maximized(struct wl_client *client, struct wl_resource *resource)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG) {
		struct nemoscreen *screen;

		nemoshell_clear_bin_next_state(bin);
		bin->requested_state.maximized = 1;
		bin->state_requested = 1;

		bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
		nemoshell_set_parent_bin(bin, NULL);

		if ((bin->has_screen == 0) &&
				(screen = nemocompz_get_main_screen(bin->shell->compz)) != NULL) {
			bin->screen.x = screen->rx;
			bin->screen.y = screen->ry;
			bin->screen.width = screen->rw;
			bin->screen.height = screen->rh;
			bin->has_screen = 1;
		}

		nemoshell_send_bin_state(bin);
	}
#endif
}

static void xdg_surface_unset_maximized(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG) {
		bin->state_requested = 1;
		bin->requested_state.maximized = 0;

		nemoshell_send_bin_state(bin);
	}
}

static void xdg_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG) {
		struct nemoscreen *screen = output_resource != NULL ? (struct nemoscreen *)wl_resource_get_user_data(output_resource) : NULL;

		nemoshell_clear_bin_next_state(bin);
		bin->requested_state.fullscreen = 1;
		bin->state_requested = 1;

		bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
		nemoshell_set_parent_bin(bin, NULL);

		if ((screen != NULL) ||
				(screen = nemocompz_get_main_screen(bin->shell->compz)) != NULL) {
			bin->screen.x = screen->rx;
			bin->screen.y = screen->ry;
			bin->screen.width = screen->rw;
			bin->screen.height = screen->rh;
			bin->has_screen = 1;
		}

		nemoshell_send_bin_state(bin);
	}
#endif
}

static void xdg_surface_unset_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG) {
		bin->state_requested = 1;
		bin->requested_state.fullscreen = 0;

		nemoshell_send_bin_state(bin);
	}
}

static void xdg_surface_set_minimized(struct wl_client *client, struct wl_resource *resource)
{
}

static const struct xdg_surface_interface xdg_surface_implementation = {
	xdg_surface_destroy,
	xdg_surface_set_parent,
	xdg_surface_set_title,
	xdg_surface_set_app_id,
	xdg_surface_show_window_menu,
	xdg_surface_move,
	xdg_surface_resize,
	xdg_surface_ack_configure,
	xdg_surface_set_window_geometry,
	xdg_surface_set_maximized,
	xdg_surface_unset_maximized,
	xdg_surface_set_fullscreen,
	xdg_surface_unset_fullscreen,
	xdg_surface_set_minimized
};

static void xdg_popup_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
}

static struct nemoclient xdg_popup_client = {
	xdg_popup_send_configure
};

static void xdg_popup_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct xdg_popup_interface xdg_popup_implementation = {
	xdg_popup_destroy
};

static void xdgshell_unbind_xdg_surface(struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->resource = NULL;
}

static void xdg_use_unstable_version(struct wl_client *client, struct wl_resource *resource, int32_t version)
{
	if (version > 1) {
		wl_resource_post_error(resource, 1, "xdg-shell: version not implemented yet.");
		return;
	}
}

static void xdg_get_xdg_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = sc->shell;
	struct shellbin *bin;
	struct clientstate *state;

	if (nemoshell_get_bin(canvas)) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wayland shell surface is already registered");
		return;
	}

	bin = nemoshell_create_bin(shell, canvas, &xdg_client);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	if (shell->default_layer != NULL)
		bin->layer = shell->default_layer;

	bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	bin->owner = sc;

	bin->resource = wl_resource_create(client, &xdg_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &xdg_surface_implementation, bin, xdgshell_unbind_xdg_surface);

	state = nemoshell_get_client_state(client);
	if (state != NULL) {
		nemoshell_set_client_state(bin, state);

		nemoshell_destroy_client_state(state);
	}
}

static void xdg_get_xdg_popup(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource *parent_resource, struct wl_resource *seat_resource, uint32_t serial, int32_t x, int32_t y, uint32_t flags)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct nemocanvas *parent = (struct nemocanvas *)wl_resource_get_user_data(parent_resource);
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = sc->shell;
	struct shellbin *bin;

	if (nemoshell_get_bin(canvas)) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wayland shell surface is already registered");
		return;
	}

	bin = nemoshell_create_bin(shell, canvas, &xdg_popup_client);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	if (shell->default_layer != NULL)
		bin->layer = shell->default_layer;

	bin->type = NEMO_SHELL_SURFACE_POPUP_TYPE;
	bin->owner = sc;
	bin->popup.x = x;
	bin->popup.y = y;
	bin->popup.serial = serial;
	nemoshell_set_parent_bin(bin, nemoshell_get_bin(parent));

	bin->resource = wl_resource_create(client, &xdg_popup_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &xdg_popup_implementation, bin, xdgshell_unbind_xdg_surface);
}

static void xdg_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	nemoshell_pong(sc, serial);
}

static const struct xdg_shell_interface xdg_implementation = {
	xdg_use_unstable_version,
	xdg_get_xdg_surface,
	xdg_get_xdg_popup,
	xdg_pong
};

static int xdgshell_dispatch(const void *implementation, void *target, uint32_t opcode, const struct wl_message *message, union wl_argument *args)
{
	struct wl_resource *resource = (struct wl_resource *)target;
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	if (opcode != 0) {
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "must call use_unstable_version first");
		return 0;
	}

#define	XDG_SERVER_VERSION	(4)

	if (args[0].i != XDG_SERVER_VERSION) {
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "incompatible version, server is %d client wants %d", XDG_SERVER_VERSION, args[0].i);
		return 0;
	}

	wl_resource_set_implementation(resource, &xdg_implementation, sc, NULL);

	return 1;
}

int xdgshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct shellclient *sc;

	sc = nemoshell_create_client(client, shell, &xdg_shell_interface, id);
	if (sc == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_dispatcher(sc->resource, xdgshell_dispatch, NULL, sc, NULL);

	return 0;
}

int xdgshell_is_xdg_surface(struct shellbin *bin)
{
	return bin && bin->resource && wl_resource_instance_of(bin->resource, &xdg_surface_interface, &xdg_surface_implementation);
}

int xdgshell_is_xdg_popup(struct shellbin *bin)
{
	return bin && bin->resource && wl_resource_instance_of(bin->resource, &xdg_popup_interface, &xdg_popup_implementation);
}
