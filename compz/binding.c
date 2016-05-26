#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <binding.h>
#include <keyboard.h>
#include <pointer.h>
#include <nemomisc.h>

struct nemobinding *nemobinding_create(uint32_t key, uint32_t button, uint32_t axis, uint32_t modifier, void *handler, void *data)
{
	struct nemobinding *binding;

	binding = (struct nemobinding *)malloc(sizeof(struct nemobinding));
	if (binding == NULL)
		return NULL;
	memset(binding, 0, sizeof(struct nemobinding));

	binding->key = key;
	binding->button = button;
	binding->axis = axis;
	binding->modifier = modifier;
	binding->handler = handler;
	binding->data = data;

	wl_list_init(&binding->link);

	return binding;
}

void nemobinding_destroy(struct nemobinding *binding)
{
	wl_list_remove(&binding->link);

	free(binding);
}

struct nemobinding *nemocompz_add_key_binding(struct nemocompz *compz, uint32_t key, uint32_t modifier, nemobinding_key_handler_t handler, void *data)
{
	struct nemobinding *binding;

	binding = nemobinding_create(key, 0, 0, modifier, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compz->key_binding_list.prev, &binding->link);

	return binding;
}

void nemocompz_run_key_binding(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state)
{
	struct nemobinding *binding;

	wl_list_for_each(binding, &compz->key_binding_list, link) {
		if (binding->key == key && nemoxkb_has_modifiers_state(keyboard->xkb, binding->modifier)) {
			nemobinding_key_handler_t handler = binding->handler;

			handler(compz, keyboard, time, key, state, binding->data);
		}
	}
}

struct nemobinding *nemocompz_add_button_binding(struct nemocompz *compz, uint32_t button, nemobinding_button_handler_t handler, void *data)
{
	struct nemobinding *binding;

	binding = nemobinding_create(0, button, 0, 0, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compz->button_binding_list.prev, &binding->link);

	return binding;
}

void nemocompz_run_button_binding(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state)
{
	struct nemobinding *binding;

	wl_list_for_each(binding, &compz->button_binding_list, link) {
		if (binding->button == button) {
			nemobinding_button_handler_t handler = binding->handler;

			handler(compz, pointer, time, button, state, binding->data);
		}
	}
}

struct nemobinding *nemocompz_add_touch_binding(struct nemocompz *compz, nemobinding_touch_handler_t handler, void *data)
{
	struct nemobinding *binding;

	binding = nemobinding_create(0, 0, 0, 0, handler, data);
	if (binding == NULL)
		return NULL;

	wl_list_insert(compz->touch_binding_list.prev, &binding->link);

	return binding;
}

void nemocompz_run_touch_binding(struct nemocompz *compz, struct touchpoint *tp, uint32_t time)
{
	struct nemobinding *binding;

	wl_list_for_each(binding, &compz->touch_binding_list, link) {
		nemobinding_touch_handler_t handler = binding->handler;

		handler(compz, tp, time, binding->data);
	}
}
