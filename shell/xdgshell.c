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
	struct wl_array configs;
	uint32_t *s;
	uint32_t serial;

	assert(bin);

	if (bin->resource == NULL)
		return;

	wl_array_init(&configs);

	if (bin->requested_config.fullscreen) {
		s = wl_array_add(&configs, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_FULLSCREEN;
	} else if (bin->requested_config.maximized) {
		s = wl_array_add(&configs, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_MAXIMIZED;
	}
	if (bin->resize_edges != 0) {
		s = wl_array_add(&configs, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_RESIZING;
	}
	if (bin->view->keyboard_count > 0) {
		s = wl_array_add(&configs, sizeof(uint32_t));
		*s = XDG_SURFACE_STATE_ACTIVATED;
	}

	serial = wl_display_next_serial(canvas->compz->display);
	xdg_surface_send_configure(bin->resource, width, height, &configs, serial);

	wl_array_release(&configs);
}

static void xdg_send_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void xdg_send_layer(struct nemocanvas *canvas, int visible)
{
}

static void xdg_send_fullscreen(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void xdg_send_close(struct nemocanvas *canvas)
{
}

static struct nemocanvas_callback xdg_callback = {
	xdg_send_configure,
	xdg_send_transform,
	xdg_send_layer,
	xdg_send_fullscreen,
	xdg_send_close
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

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MOVABLE_FLAG)) {
		nemoshell_move_canvas(bin->shell, bin, serial);
	}
}

static void xdg_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial, uint32_t edges)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_RESIZABLE_FLAG) != 0) {
		nemoshell_resize_canvas(bin->shell, bin, serial, edges);
	}
}

static void xdg_surface_ack_configure(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->config_requested) {
		bin->next_config = bin->requested_config;
		bin->config_changed = 1;
		bin->config_requested = 0;
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
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG)) {
		struct nemoscreen *screen;

		screen = nemocompz_get_main_screen(shell->compz);

		if (screen != NULL)
			nemoshell_set_maximized_bin_legacy(shell, bin, screen);
	}
#endif
}

static void xdg_surface_unset_maximized(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG)) {
		nemoshell_put_maximized_bin(shell, bin);
	}
}

static void xdg_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG)) {
		struct nemoscreen *screen = output_resource != NULL ? (struct nemoscreen *)wl_resource_get_user_data(output_resource) : NULL;

		if (screen == NULL)
			screen = nemocompz_get_main_screen(shell->compz);

		if (screen != NULL)
			nemoshell_set_fullscreen_bin_legacy(shell, bin, screen);
	}
#endif
}

static void xdg_surface_unset_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG)) {
		nemoshell_put_fullscreen_bin(shell, bin);
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

static void xdg_popup_send_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void xdg_popup_send_layer(struct nemocanvas *canvas, int visible)
{
}

static void xdg_popup_send_fullscreen(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void xdg_popup_send_close(struct nemocanvas *canvas)
{
}

static struct nemocanvas_callback xdg_popup_callback = {
	xdg_popup_send_configure,
	xdg_popup_send_transform,
	xdg_popup_send_layer,
	xdg_popup_send_fullscreen,
	xdg_popup_send_close
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

static void xdg_shell_destroy(struct wl_client *client, struct wl_resource *resource)
{
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);
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
	pid_t pid;

	if (nemoshell_get_bin(canvas)) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wayland shell surface is already registered");
		return;
	}

	bin = nemoshell_create_bin(shell, canvas, &xdg_callback);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	bin->type = NEMOSHELL_BIN_NORMAL_TYPE;
	bin->owner = sc;

	nemoshell_bin_set_state(bin, NEMOSHELL_BIN_BINDABLE_STATE);

	wl_client_get_credentials(client, &bin->pid, NULL, NULL);

	bin->resource = wl_resource_create(client, &xdg_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &xdg_surface_implementation, bin, xdgshell_unbind_xdg_surface);

	nemoshell_use_client_uuid(shell, bin);
}

static void xdg_get_xdg_popup(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource *parent_resource, struct wl_resource *seat_resource, uint32_t serial, int32_t x, int32_t y)
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

	bin = nemoshell_create_bin(shell, canvas, &xdg_popup_callback);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	bin->type = NEMOSHELL_BIN_POPUP_TYPE;
	bin->owner = sc;
	bin->popup.x = x;
	bin->popup.y = y;
	bin->popup.serial = serial;
	nemoshell_set_parent_bin(bin, nemoshell_get_bin(parent));

	wl_client_get_credentials(client, &bin->pid, NULL, NULL);

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
	xdg_shell_destroy,
	xdg_use_unstable_version,
	xdg_get_xdg_surface,
	xdg_get_xdg_popup,
	xdg_pong
};

static int xdgshell_dispatch(const void *implementation, void *target, uint32_t opcode, const struct wl_message *message, union wl_argument *args)
{
	struct wl_resource *resource = (struct wl_resource *)target;
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	if (opcode != 1) {
		wl_resource_post_error(resource, WL_DISPLAY_ERROR_INVALID_OBJECT, "must call use_unstable_version first");
		return 0;
	}

#define	XDG_SERVER_VERSION	(5)

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
