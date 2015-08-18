#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <compz.h>
#include <view.h>
#include <actor.h>
#include <screen.h>
#include <keyboard.h>
#include <keypad.h>
#include <pointer.h>
#include <touch.h>
#include <talehelper.h>
#include <nemomisc.h>

static int nemotale_dispatch_actor_pick_tale(struct nemocontent *content, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;
	float sx, sy;

	if (nemotale_get_node_count(tale) == 0) {
		return pixman_region32_contains_point(&tale->input, x, y, NULL);
	}

	return nemotale_pick(tale, x, y, &sx, &sy) != NULL;
}

static void nemotale_dispatch_actor_pointer_enter(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_enter_event(tale, 0, (uint64_t)pointer->id, pointer->x, pointer->y);
}

static void nemotale_dispatch_actor_pointer_leave(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_leave_event(tale, 0, (uint64_t)pointer->id);
}

static void nemotale_dispatch_actor_pointer_motion(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_motion_event(tale, 0, (uint64_t)pointer->id, time, x, y);
}

static void nemotale_dispatch_actor_pointer_button(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED)
		nemotale_push_pointer_down_event(tale, 0, (uint64_t)pointer->id, time, button);
	else
		nemotale_push_pointer_up_event(tale, 0, (uint64_t)pointer->id, time, button);
}

static void nemotale_dispatch_actor_keyboard_enter(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_enter_event(tale, 0, (uint64_t)keyboard->id);
}

static void nemotale_dispatch_actor_keyboard_leave(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_leave_event(tale, 0, (uint64_t)keyboard->id);
}

static void nemotale_dispatch_actor_keyboard_key(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		nemotale_push_keyboard_down_event(tale, 0, (uint64_t)keyboard->id, time, key);
	else
		nemotale_push_keyboard_up_event(tale, 0, (uint64_t)keyboard->id, time, key);
}

static void nemotale_dispatch_actor_keyboard_modifiers(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static void nemotale_dispatch_actor_keypad_enter(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_enter_event(tale, 0, (uint64_t)keypad->id);
}

static void nemotale_dispatch_actor_keypad_leave(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_leave_event(tale, 0, (uint64_t)keypad->id);
}

static void nemotale_dispatch_actor_keypad_key(struct nemokeypad *keypad, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		nemotale_push_keyboard_down_event(tale, 0, (uint64_t)keypad->id, time, key);
	else
		nemotale_push_keyboard_up_event(tale, 0, (uint64_t)keypad->id, time, key);
}

static void nemotale_dispatch_actor_keypad_modifiers(struct nemokeypad *keypad, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static void nemotale_dispatch_actor_touch_down(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_down_event(tale, 0, touchid, time, x, y);
}

static void nemotale_dispatch_actor_touch_up(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_up_event(tale, 0, touchid, time, tp->x - tp->grab_x, tp->y - tp->grab_y);
}

static void nemotale_dispatch_actor_touch_motion(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_motion_event(tale, 0, touchid, time, x, y);
}

static void nemotale_dispatch_actor_touch_frame(struct touchpoint *tp, struct nemocontent *content)
{
}

static void nemotale_dispatch_actor_dispatch_resize(struct nemoactor *actor, int32_t width, int32_t height)
{
}

void nemotale_attach_actor(struct nemotale *tale, struct nemoactor *actor, nemotale_dispatch_event_t dispatch)
{
	actor->context = (void *)tale;

	actor->base.pick = nemotale_dispatch_actor_pick_tale;

	actor->base.pointer_enter = nemotale_dispatch_actor_pointer_enter;
	actor->base.pointer_leave = nemotale_dispatch_actor_pointer_leave;
	actor->base.pointer_motion = nemotale_dispatch_actor_pointer_motion;
	actor->base.pointer_button = nemotale_dispatch_actor_pointer_button;

	actor->base.keyboard_enter = nemotale_dispatch_actor_keyboard_enter;
	actor->base.keyboard_leave = nemotale_dispatch_actor_keyboard_leave;
	actor->base.keyboard_key = nemotale_dispatch_actor_keyboard_key;
	actor->base.keyboard_modifiers = nemotale_dispatch_actor_keyboard_modifiers;

	actor->base.keypad_enter = nemotale_dispatch_actor_keypad_enter;
	actor->base.keypad_leave = nemotale_dispatch_actor_keypad_leave;
	actor->base.keypad_key = nemotale_dispatch_actor_keypad_key;
	actor->base.keypad_modifiers = nemotale_dispatch_actor_keypad_modifiers;

	actor->base.touch_down = nemotale_dispatch_actor_touch_down;
	actor->base.touch_up = nemotale_dispatch_actor_touch_up;
	actor->base.touch_motion = nemotale_dispatch_actor_touch_motion;
	actor->base.touch_frame = nemotale_dispatch_actor_touch_frame;

	actor->dispatch_resize = nemotale_dispatch_actor_dispatch_resize;

	nemotale_set_dispatch_event(tale, dispatch);
}
