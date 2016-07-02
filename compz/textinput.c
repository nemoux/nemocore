#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-text-server-protocol.h>
#include <wayland-input-method-server-protocol.h>

#include <textinput.h>
#include <inputmethod.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <seat.h>
#include <keyboard.h>
#include <nemomisc.h>

static void text_input_activate(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, struct wl_resource *surface_resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_deactivate(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_show_input_panel(struct wl_client *client, struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_hide_input_panel(struct wl_client *client, struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_reset(struct wl_client *client, struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_set_surrounding_text(struct wl_client *client, struct wl_resource *resource, const char *text, uint32_t cursor, uint32_t anchor)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_set_content_type(struct wl_client *client, struct wl_resource *resource, uint32_t hint, uint32_t purpose)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_set_cursor_rectangle(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_set_preferred_language(struct wl_client *client, struct wl_resource *resource, const char *language)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_invoke_action(struct wl_client *client, struct wl_resource *resource, uint32_t button, uint32_t index)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static void text_input_commit_state(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
}

static const struct wl_text_input_interface text_input_implementation = {
	text_input_activate,
	text_input_deactivate,
	text_input_show_input_panel,
	text_input_hide_input_panel,
	text_input_reset,
	text_input_set_surrounding_text,
	text_input_set_content_type,
	text_input_set_cursor_rectangle,
	text_input_set_preferred_language,
	text_input_commit_state,
	text_input_invoke_action
};

static void textinput_unbind_text_input(struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);

	free(textinput);
}

static void text_input_manager_create_text_input(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
	struct textmanager *textmanager = (struct textmanager *)wl_resource_get_user_data(resource);
	struct textinput *textinput;

	textinput = (struct textinput *)malloc(sizeof(struct textinput));
	if (textinput == NULL) {
		wl_client_post_no_memory(client);
		return;
	}
	memset(textinput, 0, sizeof(struct textinput));

	textinput->compz = textmanager->compz;

	textinput->resource = wl_resource_create(client, &wl_text_input_interface, 1, id);
	if (textinput->resource == NULL) {
		wl_client_post_no_memory(client);
		free(textinput);
		return;
	}

	wl_resource_set_implementation(textinput->resource, &text_input_implementation, textinput, textinput_unbind_text_input);
}

static const struct wl_text_input_manager_interface text_input_manager_implementation = {
	text_input_manager_create_text_input
};

static void textinput_bind_manager(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct textmanager *textmanager = (struct textmanager *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_text_input_manager_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &text_input_manager_implementation, textmanager, NULL);
}

static void textinput_handle_compz_destroy(struct wl_listener *listener, void *data)
{
	struct textmanager *textmanager = (struct textmanager *)container_of(listener, struct textmanager, destroy_listener);

	wl_global_destroy(textmanager->global);

	wl_list_remove(&textmanager->destroy_listener.link);

	free(textmanager);
}

struct textmanager *textinput_create_manager(struct nemocompz *compz)
{
	struct textmanager *textmanager;

	textmanager = (struct textmanager *)malloc(sizeof(struct textmanager));
	if (textmanager == NULL)
		return NULL;
	memset(textmanager, 0, sizeof(struct textmanager));

	textmanager->compz = compz;

	textmanager->global = wl_global_create(compz->display, &wl_text_input_manager_interface, 1, textmanager, textinput_bind_manager);
	if (textmanager->global == NULL)
		goto err1;

	textmanager->destroy_listener.notify = textinput_handle_compz_destroy;
	wl_signal_add(&compz->destroy_signal, &textmanager->destroy_listener);

	return textmanager;

err1:
	free(textmanager);

	return NULL;
}
