#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>

#include <waylandshell.h>
#include <shell.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <screen.h>
#include <move.h>
#include <resize.h>
#include <nemomisc.h>

static void shell_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	assert(bin);

	if (bin->resource) {
		wl_shell_surface_send_configure(bin->resource, bin->resize_edges, width, height);
	}
}

static void shell_send_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void shell_send_layer(struct nemocanvas *canvas, int visible)
{
}

static void shell_send_fullscreen(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void shell_send_close(struct nemocanvas *canvas)
{
}

static struct nemocanvas_callback shell_callback = {
	shell_send_configure,
	shell_send_transform,
	shell_send_layer,
	shell_send_fullscreen,
	shell_send_close
};

static void shell_surface_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct shellclient *sc = bin->owner;

	nemoshell_pong(sc, serial);
}

static void shell_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MOVABLE_FLAG)) {
		nemoshell_move_canvas(bin->shell, bin, serial);
	}
}

static void shell_surface_resize(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial, uint32_t edges)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_RESIZABLE_FLAG) != 0) {
		nemoshell_resize_canvas(bin->shell, bin, serial, edges);
	}
}

static void shell_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	nemoshell_clear_bin_config(bin);

	bin->type = NEMOSHELL_BIN_NORMAL_TYPE;

	nemoshell_set_parent_bin(bin, NULL);
}

static void shell_surface_set_transient(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent_resource, int x, int y, uint32_t flags)
{
}

static void shell_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t method, uint32_t framerate, struct wl_resource *output_resource)
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

static void shell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemocanvas *parent = (struct nemocanvas *)wl_resource_get_user_data(parent_resource);
	struct nemoshell *shell = bin->shell;

	nemoshell_clear_bin_config(bin);

	bin->type = NEMOSHELL_BIN_POPUP_TYPE;
	bin->popup.x = x;
	bin->popup.y = y;
	bin->popup.serial = serial;

	nemoshell_set_parent_bin(bin, nemoshell_get_bin(parent));
}

static void shell_surface_set_maximized(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG)) {
		struct nemoscreen *screen = output_resource != NULL ? (struct nemoscreen *)wl_resource_get_user_data(output_resource) : NULL;

		if (screen == NULL)
			screen = nemocompz_get_main_screen(shell->compz);

		if (screen != NULL)
			nemoshell_set_maximized_bin_legacy(shell, bin, screen);
	}
#endif
}

static void shell_surface_set_title(struct wl_client *client, struct wl_resource *resource, const char *title)
{
}

static void shell_surface_set_class(struct wl_client *client, struct wl_resource *resource, const char *class)
{
}

static const struct wl_shell_surface_interface shell_surface_implementation = {
	shell_surface_pong,
	shell_surface_move,
	shell_surface_resize,
	shell_surface_set_toplevel,
	shell_surface_set_transient,
	shell_surface_set_fullscreen,
	shell_surface_set_popup,
	shell_surface_set_maximized,
	shell_surface_set_title,
	shell_surface_set_class
};

static void shell_unbind_shell_surface(struct wl_resource *resource)
{
}

static void shell_get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = sc->shell;
	struct shellbin *bin;

	if (nemoshell_get_bin(canvas)) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wayland shell surface is already registered");
		return;
	}

	bin = nemoshell_create_bin(shell, canvas, &shell_callback);
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

	bin->resource = wl_resource_create(client, &wl_shell_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &shell_surface_implementation, bin, shell_unbind_shell_surface);

	nemoshell_use_client_uuid(shell, bin);
}

static const struct wl_shell_interface shell_implementation = {
	shell_get_shell_surface
};

int waylandshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct shellclient *sc;

	sc = nemoshell_create_client(client, shell, &wl_shell_interface, id);
	if (sc == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(sc->resource, &shell_implementation, sc, NULL);

	return 0;
}

int waylandshell_is_shell_surface(struct shellbin *bin)
{
	return bin && bin->resource && wl_resource_instance_of(bin->resource, &wl_shell_surface_interface, &shell_surface_implementation);
}
