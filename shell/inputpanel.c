#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-input-method-server-protocol.h>

#include <inputpanel.h>
#include <shell.h>
#include <compz.h>
#include <view.h>
#include <canvas.h>
#include <screen.h>
#include <nemolog.h>
#include <nemomisc.h>

static void inputpanel_show_bin(struct inputbin *bin)
{
	nemoview_attach_layer(bin->view, bin->layer);
	nemoview_damage_below(bin->view);

	nemoview_update_transform(bin->view);
}

static void inputpanel_hide_bin(struct inputbin *bin)
{
	nemoview_unmap(bin->view);
}

static void inputpanel_handle_show_input_panel(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, show_input_panel_listener);
	struct inputbin *bin, *nbin;

	shell->textinput.canvas = (struct nemocanvas *)data;

	if (shell->showing_input_panels)
		return;

	shell->showing_input_panels = 1;

	wl_list_for_each_safe(bin, nbin, &shell->inputpanel.bin_list, link) {
		if (bin->canvas->base.width == 0)
			continue;

		inputpanel_show_bin(bin);
	}
}

static void inputpanel_handle_hide_input_panel(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, hide_input_panel_listener);
	struct inputbin *bin, *nbin;

	if (!shell->showing_input_panels)
		return;

	shell->showing_input_panels = 0;

	wl_list_for_each_safe(bin, nbin, &shell->inputpanel.bin_list, link) {
		inputpanel_hide_bin(bin);
	}
}

static void inputpanel_handle_update_input_panel(struct wl_listener *listener, void *data)
{
	struct nemoshell *shell = (struct nemoshell *)container_of(listener, struct nemoshell, update_input_panel_listener);

	memcpy(&shell->textinput.cursor, data, sizeof(pixman_box32_t));
}

static void inputbin_configure_canvas(struct nemocanvas *canvas, int32_t sx, int32_t sy)
{
	struct inputbin *bin = (struct inputbin *)canvas->configure_private;
	struct nemoshell *shell = bin->shell;
	struct nemoview *view;
	int32_t x, y;

	if (canvas->base.width == 0)
		return;

	if (bin->panel) {
		view = nemoshell_get_default_view(shell->textinput.canvas);
		if (view == NULL)
			return;

		x = view->geometry.x + shell->textinput.cursor.x2;
		y = view->geometry.y + shell->textinput.cursor.y2;
	} else {
		x = bin->screen.x + (bin->screen.width - bin->canvas->base.width) / 2;
		y = bin->screen.y + bin->screen.height - bin->canvas->base.height;
	}

	nemoview_set_position(bin->view, x, y);

	if (shell->showing_input_panels &&
			!nemoview_has_state(bin->view, NEMO_VIEW_MAP_STATE)) {
		nemoview_attach_layer(bin->view, bin->layer);
		nemoview_damage_below(bin->view);

		nemoview_update_transform(bin->view);
	}
}

static void inputpanel_destroy_bin(struct inputbin *bin);

static void inputpanel_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct inputbin *bin = (struct inputbin *)container_of(listener, struct inputbin, canvas_destroy_listener);

	if (bin->resource != NULL) {
		wl_resource_destroy(bin->resource);
	} else {
		inputpanel_destroy_bin(bin);
	}
}

static struct inputbin *inputpanel_create_bin(struct nemoshell *shell, struct nemocanvas *canvas)
{
	struct inputbin *bin;

	bin = (struct inputbin *)malloc(sizeof(struct inputbin));
	if (bin == NULL)
		return NULL;
	memset(bin, 0, sizeof(struct inputbin));

	bin->view = nemoview_create(canvas->compz, &canvas->base);
	if (bin->view == NULL)
		goto err1;

	bin->view->canvas = canvas;

	wl_list_insert(&canvas->view_list, &bin->view->link);

	bin->shell = shell;
	bin->canvas = canvas;
	bin->layer = &shell->service_layer;

	canvas->configure = inputbin_configure_canvas;
	canvas->configure_private = (void *)bin;

	wl_signal_init(&bin->destroy_signal);

	bin->canvas_destroy_listener.notify = inputpanel_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &bin->canvas_destroy_listener);

	wl_list_init(&bin->link);

	return bin;

err1:
	free(bin);

	return NULL;
}

static void inputpanel_destroy_bin(struct inputbin *bin)
{
	wl_signal_emit(&bin->destroy_signal, bin);

	wl_list_remove(&bin->canvas_destroy_listener.link);
	wl_list_remove(&bin->link);

	bin->canvas->configure = NULL;

	nemoview_destroy(bin->view);

	free(bin);
}

static void input_panel_surface_set_toplevel(struct wl_client *client, struct wl_resource *resource, struct wl_resource *output_resource, uint32_t position)
{
	struct inputbin *bin = (struct inputbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;
	struct nemoscreen *screen;

	wl_list_insert(&shell->inputpanel.bin_list, &bin->link);

	screen = (struct nemoscreen *)wl_resource_get_user_data(output_resource);
	if (screen != NULL) {
		bin->screen.x = screen->x;
		bin->screen.y = screen->y;
		bin->screen.width = screen->width;
		bin->screen.height = screen->height;
		bin->panel = 0;
	}
}

static void input_panel_surface_set_overlay_panel(struct wl_client *client, struct wl_resource *resource)
{
	struct inputbin *bin = (struct inputbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	wl_list_insert(&shell->inputpanel.bin_list, &bin->link);

	bin->panel = 1;
}

static const struct wl_input_panel_surface_interface input_panel_surface_implementation = {
	input_panel_surface_set_toplevel,
	input_panel_surface_set_overlay_panel
};

static void inputpanel_unbind_input_panel_surface(struct wl_resource *resource)
{
	struct inputbin *bin = (struct inputbin *)wl_resource_get_user_data(resource);

	inputpanel_destroy_bin(bin);
}

static void input_panel_get_input_panel_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
	struct nemoshell *shell = (struct nemoshell *)wl_resource_get_user_data(resource);
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct inputbin *bin;

	if (inputpanel_get_bin(canvas) != NULL) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wl_input_panel::get_input_panel_surface is already requested");
		return;
	}

	bin = inputpanel_create_bin(shell, canvas);
	if (bin == NULL) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create input panel surface");
		return;
	}

	bin->resource = wl_resource_create(client, &wl_input_panel_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_client_post_no_memory(client);
		inputpanel_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &input_panel_surface_implementation, bin, inputpanel_unbind_input_panel_surface);
}

static const struct wl_input_panel_interface input_panel_implementation = {
	input_panel_get_input_panel_surface
};

static void inputpanel_unbind_input_panel(struct wl_resource *resource)
{
	struct nemoshell *shell = (struct nemoshell *)wl_resource_get_user_data(resource);

	shell->inputpanel.binding = NULL;
}

static void inputpanel_bind_input_panel(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct wl_resource *resource;

	if (shell->inputpanel.binding != NULL) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"input panel is already bounded");
		return;
	}

	resource = wl_resource_create(client, &wl_input_panel_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &input_panel_implementation, shell, inputpanel_unbind_input_panel);

	shell->inputpanel.binding = resource;
}

int inputpanel_prepare(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;

	shell->show_input_panel_listener.notify = inputpanel_handle_show_input_panel;
	wl_signal_add(&compz->show_input_panel_signal, &shell->show_input_panel_listener);

	shell->hide_input_panel_listener.notify = inputpanel_handle_hide_input_panel;
	wl_signal_add(&compz->hide_input_panel_signal, &shell->hide_input_panel_listener);

	shell->update_input_panel_listener.notify = inputpanel_handle_update_input_panel;
	wl_signal_add(&compz->update_input_panel_signal, &shell->update_input_panel_listener);

	wl_list_init(&shell->inputpanel.bin_list);

	if (!wl_global_create(compz->display, &wl_input_panel_interface, 1, shell, inputpanel_bind_input_panel)) {
		return -1;
	}

	return 0;
}

void inputpanel_finish(struct nemoshell *shell)
{
	struct inputbin *bin, *nbin;

	if (shell->inputpanel.binding != NULL) {
		wl_resource_destroy(shell->inputpanel.binding);
	}

	wl_list_for_each_safe(bin, nbin, &shell->inputpanel.bin_list, link) {
		if (bin->resource != NULL) {
			wl_resource_destroy(bin->resource);
		} else {
			inputpanel_destroy_bin(bin);
		}
	}

	wl_list_remove(&shell->show_input_panel_listener.link);
	wl_list_remove(&shell->hide_input_panel_listener.link);
	wl_list_remove(&shell->update_input_panel_listener.link);
}

struct inputbin *inputpanel_get_bin(struct nemocanvas *canvas)
{
	if (canvas->configure == inputbin_configure_canvas) {
		return canvas->configure_private;
	}

	return NULL;
}
