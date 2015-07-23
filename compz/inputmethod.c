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
#include <textbackend.h>
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

static void input_method_context_unbind_keyboard(struct wl_resource *resource)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	inputmethod_end_keyboard_grab(context);

	context->keyboard = NULL;
}

static void input_method_context_grab_key(struct nemokeyboard_grab *grab, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemokeyboard *keyboard = grab->keyboard;
	struct wl_display *display;
	uint32_t serial;

	if (keyboard->inputmethod.resource == NULL)
		return;

	display = wl_client_get_display(wl_resource_get_client(keyboard->inputmethod.resource));
	serial = wl_display_next_serial(display);
	wl_keyboard_send_key(keyboard->inputmethod.resource, serial, time, key, state);
}

static void input_method_context_grab_modifiers(struct nemokeyboard_grab *grab, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct nemokeyboard *keyboard = grab->keyboard;
	struct wl_display *display;
	uint32_t serial;

	if (keyboard->inputmethod.resource == NULL)
		return;

	display = wl_client_get_display(wl_resource_get_client(keyboard->inputmethod.resource));
	serial = wl_display_next_serial(display);
	wl_keyboard_send_modifiers(keyboard->inputmethod.resource, serial, mods_depressed, mods_latched, mods_locked, group);
}

static void input_method_context_grab_cancel(struct nemokeyboard_grab *grab)
{
	nemokeyboard_end_grab(grab->keyboard);
}

static const struct nemokeyboard_grab_interface input_method_context_grab_interface = {
	input_method_context_grab_key,
	input_method_context_grab_modifiers,
	input_method_context_grab_cancel,
};

static void input_method_context_grab_keyboard(struct wl_client *client, struct wl_resource *context_resource, uint32_t id)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(context_resource);
	struct wl_resource *resource;
	struct nemoseat *seat = context->inputmethod->seat;
	struct nemokeyboard *keyboard;

	keyboard = nemoseat_get_first_keyboard(seat);
	if (keyboard == NULL) {
		wl_resource_post_error(context_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to find keyboard");
		return;
	}

	resource = wl_resource_create(client, &wl_keyboard_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, NULL, context, input_method_context_unbind_keyboard);

	context->keyboard = resource;

	wl_keyboard_send_keymap(resource, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, keyboard->xkb->xkbinfo->keymap_fd, keyboard->xkb->xkbinfo->keymap_size);

	if (keyboard->grab != &keyboard->default_grab) {
		nemokeyboard_end_grab(keyboard);
	}

	keyboard->inputmethod.grab.interface = &input_method_context_grab_interface;
	nemokeyboard_start_grab(keyboard, &keyboard->inputmethod.grab);

	keyboard->inputmethod.resource = resource;
}

static void input_method_context_key(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);
	struct nemoseat *seat = context->inputmethod->seat;
	struct nemokeyboard *keyboard;
	struct nemokeyboard_grab *grab = &keyboard->default_grab;

	keyboard = nemoseat_get_first_keyboard(seat);
	if (keyboard == NULL) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to find keyboard");
		return;
	}

	grab->interface->key(grab, time, key, state);
}

static void input_method_context_modifiers(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);
	struct nemoseat *seat = context->inputmethod->seat;
	struct nemokeyboard *keyboard;
	struct nemokeyboard_grab *grab = &keyboard->default_grab;

	keyboard = nemoseat_get_first_keyboard(seat);
	if (keyboard == NULL) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to find keyboard");
		return;
	}

	grab->interface->modifiers(grab, mods_depressed, mods_latched, mods_locked, group);
}

static void input_method_context_language(struct wl_client *client, struct wl_resource *resource, uint32_t serial, const char *language)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_language(context->model->resource, serial, language);
	}
}

static void input_method_context_text_direction(struct wl_client *client, struct wl_resource *resource, uint32_t serial, uint32_t direction)
{
	struct inputcontext *context = (struct inputcontext *)wl_resource_get_user_data(resource);

	if (context->model) {
		wl_text_input_send_text_direction(context->model->resource, serial, direction);
	}
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

static void inputmethod_handle_keyboard_focus(struct wl_listener *listener, void *data)
{
	struct inputmethod *inputmethod = (struct inputmethod *)container_of(listener, struct inputmethod, keyboard_focus_listener);
	struct nemokeyboard *keyboard = (struct nemokeyboard *)data;
	struct nemoview *focus = keyboard->focus;

	if (inputmethod->model == NULL)
		return;

	if (focus == NULL || focus->canvas == NULL)
		return;

	if (inputmethod->model->canvas != focus->canvas) {
		textinput_deactivate_input_method(inputmethod->model, inputmethod);
	}
}

static void inputmethod_unbind_input_method(struct wl_resource *resource)
{
	struct inputmethod *inputmethod = (struct inputmethod *)wl_resource_get_user_data(resource);
	struct textbackend *textbackend = inputmethod->textbackend;

	inputmethod->binding = NULL;
	inputmethod->context = NULL;

	textbackend->inputmethod.binding = NULL;
}

static void inputmethod_bind_input_method(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct inputmethod *inputmethod = (struct inputmethod *)data;
	struct textbackend *textbackend = inputmethod->textbackend;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_input_method_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	if (inputmethod->binding != NULL) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"input method is already bounded");
		wl_resource_destroy(resource);
		return;
	}

	if (textbackend->inputmethod.client != client) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"input method binding is denied");
		wl_resource_destroy(resource);
		return;
	}

	wl_resource_set_implementation(resource, NULL, inputmethod, inputmethod_unbind_input_method);

	inputmethod->binding = resource;

	textbackend->inputmethod.binding = resource;
}

static void inputmethod_handle_seat_destroy(struct wl_listener *listener, void *data)
{
	struct inputmethod *inputmethod = (struct inputmethod *)container_of(listener, struct inputmethod, destroy_listener);

	wl_global_destroy(inputmethod->global);
	wl_list_remove(&inputmethod->destroy_listener.link);

	free(inputmethod);
}

struct inputmethod *inputmethod_create(struct nemoseat *seat, struct textbackend *textbackend)
{
	struct inputmethod *inputmethod;

	inputmethod = (struct inputmethod *)malloc(sizeof(struct inputmethod));
	if (inputmethod == NULL)
		return NULL;
	memset(inputmethod, 0, sizeof(struct inputmethod));

	inputmethod->seat = seat;
	inputmethod->model = NULL;
	inputmethod->context = NULL;
	inputmethod->textbackend = textbackend;

	inputmethod->global = wl_global_create(seat->compz->display, &wl_input_method_interface, 1, inputmethod, inputmethod_bind_input_method);
	if (inputmethod->global == NULL)
		goto err1;

	inputmethod->destroy_listener.notify = inputmethod_handle_seat_destroy;
	wl_signal_add(&seat->destroy_signal, &inputmethod->destroy_listener);

	inputmethod->keyboard_focus_listener.notify = inputmethod_handle_keyboard_focus;
	wl_signal_add(&seat->keyboard.focus_signal, &inputmethod->keyboard_focus_listener);

	seat->inputmethod = inputmethod;

	return inputmethod;

err1:
	free(inputmethod);

	return NULL;
}

void inputmethod_prepare_keyboard_grab(struct nemokeyboard *keyboard)
{
	keyboard->inputmethod.grab.interface = &input_method_context_grab_interface;
}

void inputmethod_end_keyboard_grab(struct inputcontext *context)
{
	struct nemokeyboard_grab *grab;
	struct nemokeyboard *keyboard;

	keyboard = nemoseat_get_first_keyboard(context->inputmethod->seat);
	if (keyboard == NULL)
		return;

	grab = &keyboard->inputmethod.grab;

	if (grab->keyboard == NULL)
		return;

	if (grab->keyboard->grab == grab)
		nemokeyboard_end_grab(keyboard);

	keyboard->inputmethod.resource = NULL;
}
