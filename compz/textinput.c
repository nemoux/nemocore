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
#include <textbackend.h>
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
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct nemocompz *compz = textinput->compz;
	struct nemokeyboard *keyboard;
	struct inputmethod *inputmethod = seat->inputmethod;
	struct textinput *old = inputmethod->model;

	if (old == textinput)
		return;

	if (old != NULL) {
		textinput_deactivate_input_method(old, inputmethod);
	}

	keyboard = nemoseat_get_first_keyboard(seat);
	if (keyboard == NULL)
		return;

	inputmethod->model = textinput;
	wl_list_insert(&textinput->inputmethod_list, &inputmethod->link);
	inputmethod_prepare_keyboard_grab(keyboard);

	textinput->canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);

	inputmethod_create_context(inputmethod, textinput);

	if (textinput->input_panel_visible) {
		wl_signal_emit(&compz->show_input_panel_signal, textinput->canvas);
		wl_signal_emit(&compz->update_input_panel_signal, &textinput->cursor);
	}

	wl_text_input_send_enter(textinput->resource, textinput->canvas->resource);
}

static void text_input_deactivate(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);

	textinput_deactivate_input_method(textinput, seat->inputmethod);
}

static void text_input_show_input_panel(struct wl_client *client, struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct nemocompz *compz = textinput->compz;

	textinput->input_panel_visible = 1;

	if (!wl_list_empty(&textinput->inputmethod_list)) {
		wl_signal_emit(&compz->show_input_panel_signal, textinput->canvas);
		wl_signal_emit(&compz->update_input_panel_signal, &textinput->cursor);
	}
}

static void text_input_hide_input_panel(struct wl_client *client, struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct nemocompz *compz = textinput->compz;

	textinput->input_panel_visible = 0;

	if (!wl_list_empty(&textinput->inputmethod_list)) {
		wl_signal_emit(&compz->hide_input_panel_signal, compz);
	}
}

static void text_input_reset(struct wl_client *client, struct wl_resource *resource)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		if (inputmethod->context == NULL)
			continue;

		wl_input_method_context_send_reset(inputmethod->context->resource);
	}
}

static void text_input_set_surrounding_text(struct wl_client *client, struct wl_resource *resource, const char *text, uint32_t cursor, uint32_t anchor)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		if (inputmethod->context == NULL)
			continue;

		wl_input_method_context_send_surrounding_text(inputmethod->context->resource, text, cursor, anchor);
	}
}

static void text_input_set_content_type(struct wl_client *client, struct wl_resource *resource, uint32_t hint, uint32_t purpose)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		if (inputmethod->context == NULL)
			continue;

		wl_input_method_context_send_content_type(inputmethod->context->resource, hint, purpose);
	}
}

static void text_input_set_cursor_rectangle(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct nemocompz *compz = textinput->compz;

	textinput->cursor.x1 = x;
	textinput->cursor.y1 = y;
	textinput->cursor.x2 = x + width;
	textinput->cursor.y2 = y + height;

	wl_signal_emit(&compz->update_input_panel_signal, &textinput->cursor);
}

static void text_input_set_preferred_language(struct wl_client *client, struct wl_resource *resource, const char *language)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		if (inputmethod->context == NULL)
			continue;

		wl_input_method_context_send_preferred_language(inputmethod->context->resource, language);
	}
}

static void text_input_invoke_action(struct wl_client *client, struct wl_resource *resource, uint32_t button, uint32_t index)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		if (inputmethod->context == NULL)
			continue;

		wl_input_method_context_send_invoke_action(inputmethod->context->resource, button, index);
	}
}

static void text_input_commit_state(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct textinput *textinput = (struct textinput *)wl_resource_get_user_data(resource);
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		if (inputmethod->context == NULL)
			continue;

		wl_input_method_context_send_commit_state(inputmethod->context->resource, serial);
	}
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
	struct inputmethod *inputmethod, *next;

	wl_list_for_each_safe(inputmethod, next, &textinput->inputmethod_list, link) {
		textinput_deactivate_input_method(textinput, inputmethod);
	}

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

	wl_list_init(&textinput->inputmethod_list);

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

void textinput_deactivate_input_method(struct textinput *textinput, struct inputmethod *inputmethod)
{
	struct nemocompz *compz = textinput->compz;

	if (inputmethod->model == textinput) {
		if (inputmethod->context && inputmethod->binding) {
			inputmethod_end_keyboard_grab(inputmethod->context);
			wl_input_method_send_deactivate(inputmethod->binding, inputmethod->context->resource);
			inputmethod->context->model = NULL;
		}

		wl_list_remove(&inputmethod->link);
		inputmethod->model = NULL;
		inputmethod->context = NULL;
		wl_signal_emit(&compz->hide_input_panel_signal, compz);
		wl_text_input_send_leave(textinput->resource);
	}
}
