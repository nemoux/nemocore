#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-text-server-protocol.h>
#include <wayland-input-method-server-protocol.h>

#include <inputmethod.h>
#include <textinput.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <seat.h>
#include <keyboard.h>
#include <keymap.h>
#include <nemomisc.h>
#include <nemolog.h>

static void input_method_context_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void input_method_context_commit_string(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *text)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_commit_string(context->model->resource, serial, text);
	}
}

static void input_method_context_preedit_string(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *text, const char *commit)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_preedit_string(context->model->resource, serial, text, commit);
	}
}

static void input_method_context_preedit_styling(struct wl_client *client, struct wl_resource *resource, uint32_t index, uint32_t length, uint32_t style)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_preedit_styling(context->model->resource, index, length, style);
	}
}

static void input_method_context_preedit_cursor(struct wl_client *client, struct wl_resource *resource, int32_t cursor)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_preedit_cursor(context->model->resource, cursor);
	}
}

static void input_method_context_delete_surrounding_text(struct wl_client *client, struct wl_resource *resource, int32_t index, uint32_t length)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_delete_surrounding_text(context->model->resource, index, length);
	}
}

static void input_method_context_cursor_position(struct wl_client *client, struct wl_resource *resource, int32_t index, int32_t anchor)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_cursor_position(context->model->resource, index, anchor);
	}
}

static void input_method_context_modifiers_map(struct wl_client *client, struct wl_resource *resource, struct wl_array *map)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_modifiers_map(context->model->resource, map);
	}
}

static void input_method_context_keysym(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t time, uint32_t sym, uint32_t state, uint32_t modifiers)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_keysym(context->model->resource, serial, time, sym, state, modifiers);
	}
}

static void input_method_context_key(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);
}

static void input_method_context_modifiers(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);
}

static void input_method_context_language(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *language)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);
}

static void input_method_context_text_direction(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t direction)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);
}

static const struct wl_input_method_context_interface input_method_context_implementation = {
	input_method_context_destroy,
	input_method_context_commit_string,
	input_method_context_preedit_string,
	input_method_context_preedit_styling,
	input_method_context_preedit_cursor,
	input_method_context_delete_surrounding_text,
	input_method_context_cursor_position,
	input_method_context_modifiers_map,
	input_method_context_keysym,
	input_method_context_grab_keyboard,
	input_method_context_key,
	input_method_context_modifiers,
	input_method_context_language,
	input_method_context_text_direction
};

static void inputmethod_unbind_context(struct wl_resource *resource)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->keyboard != NULL) {
		wl_resource_destroy(context->keyboard);
	}

	free(context);
}

void inputmethod_create_context(struct inputmethod *inputmethod, struct textinput *textinput)
{
	struct inputcontext *context;

	if (inputmethod->binding == NULL)
		return;

	context = (struct inputcontext *)malloc(sizeof(struct inputcontext));
	if (context == NULL)
		return;
	memset(context, 0, sizeof(struct inputcontext));

	context->resource = wl_resource_create(
			wl_resource_get_client(inputmethod->binding),
			&wl_input_method_context_interface, 1, 0);
	if (context->resource == NULL) {
		free(context);
		return;
	}

	wl_resource_set_implementation(context->resource, &input_method_context_implementation, context, inputmethod_unbind_context);

	context->model = textinput;
	context->inputmethod = inputmethod;

	inputmethod->context = context;

	wl_input_method_send_activate(inputmethod->binding, context->resource);
}
