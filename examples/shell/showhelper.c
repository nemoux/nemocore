#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <view.h>
#include <actor.h>
#include <screen.h>
#include <keyboard.h>
#include <keypad.h>
#include <pointer.h>
#include <touch.h>
#include <timer.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <nemomisc.h>

static int nemoshow_dispatch_pick_tale(struct nemocontent *content, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;
	float sx, sy;

	if (nemotale_get_node_count(tale) == 0) {
		return pixman_region32_contains_point(&tale->input, x, y, NULL);
	}

	return nemotale_pick(tale, x, y, &sx, &sy) != NULL;
}

static void nemoshow_dispatch_pointer_enter(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_enter_event(tale, 0, (uint64_t)pointer->id, pointer->x, pointer->y);
}

static void nemoshow_dispatch_pointer_leave(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_leave_event(tale, 0, (uint64_t)pointer->id);
}

static void nemoshow_dispatch_pointer_motion(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_motion_event(tale, 0, (uint64_t)pointer->id, time, x, y);
}

static void nemoshow_dispatch_pointer_axis(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t axis, float value)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_pointer_axis_event(tale, 0, (uint64_t)pointer->id, time, axis, value);
}

static void nemoshow_dispatch_pointer_button(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	if (state == WL_POINTER_BUTTON_STATE_PRESSED)
		nemotale_push_pointer_down_event(tale, 0, (uint64_t)pointer->id, time, button);
	else
		nemotale_push_pointer_up_event(tale, 0, (uint64_t)pointer->id, time, button);
}

static void nemoshow_dispatch_keyboard_enter(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_enter_event(tale, 0, (uint64_t)keyboard->id);
}

static void nemoshow_dispatch_keyboard_leave(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_leave_event(tale, 0, (uint64_t)keyboard->id);
}

static void nemoshow_dispatch_keyboard_key(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		nemotale_push_keyboard_down_event(tale, 0, (uint64_t)keyboard->id, time, key);
	else
		nemotale_push_keyboard_up_event(tale, 0, (uint64_t)keyboard->id, time, key);
}

static void nemoshow_dispatch_keyboard_modifiers(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static void nemoshow_dispatch_keypad_enter(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_enter_event(tale, 0, (uint64_t)keypad->id);
}

static void nemoshow_dispatch_keypad_leave(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_keyboard_leave_event(tale, 0, (uint64_t)keypad->id);
}

static void nemoshow_dispatch_keypad_key(struct nemokeypad *keypad, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
		nemotale_push_keyboard_down_event(tale, 0, (uint64_t)keypad->id, time, key);
	else
		nemotale_push_keyboard_up_event(tale, 0, (uint64_t)keypad->id, time, key);
}

static void nemoshow_dispatch_keypad_modifiers(struct nemokeypad *keypad, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static void nemoshow_dispatch_touch_down(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_down_event(tale, 0, touchid, time, x, y);
}

static void nemoshow_dispatch_touch_up(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_up_event(tale, 0, touchid, time, tp->x - tp->grab_x, tp->y - tp->grab_y);
}

static void nemoshow_dispatch_touch_motion(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_motion_event(tale, 0, touchid, time, x, y);
}

static void nemoshow_dispatch_touch_frame(struct touchpoint *tp, struct nemocontent *content)
{
}

static void nemoshow_dispatch_actor_resize(struct nemoactor *actor, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoactor_resize_gl(actor, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_render_one(show);

	nemotale_composite_fbo_full(scon->tale);
}

static void nemoshow_dispatch_actor_frame(struct nemoactor *actor, uint32_t msecs)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct nemocompz *compz = actor->compz;
	pixman_region32_t region;

	pixman_region32_init(&region);

	nemocompz_make_current(compz);

	if (msecs == 0) {
		nemoactor_feedback(actor);
	} else if (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, msecs);

		nemoactor_feedback(actor);
	} else {
		nemoactor_feedback_done(actor);
	}

	nemoshow_render_one(show);

	nemotale_composite_fbo(tale, &region);

	nemoactor_damage_region(actor, &region);

	pixman_region32_fini(&region);
}

static void nemoshow_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct showcontext *scon = (struct showcontext *)data;

	nemoshow_validate_one(scon->show);

	nemotimer_set_timeout(timer, 1000);
}

struct nemoshow *nemoshow_create_on_actor(struct nemoshell *shell, int32_t width, int32_t height, nemotale_dispatch_event_t dispatch)
{
	struct nemocompz *compz = shell->compz;
	struct showcontext *scon;
	struct nemoshow *show;
	struct nemoactor *actor;
	struct nemotimer *timer;

	scon = (struct showcontext *)malloc(sizeof(struct showcontext));
	if (scon == NULL)
		return NULL;
	memset(scon, 0, sizeof(struct showcontext));

	actor = nemoactor_create_gl(compz, width, height);
	if (actor == NULL)
		goto err1;

	timer = nemotimer_create(compz);
	if (timer == NULL)
		goto err2;
	nemotimer_set_callback(timer, nemoshow_dispatch_timer);
	nemotimer_set_timeout(timer, 1000);
	nemotimer_set_userdata(timer, scon);

	actor->base.pick = nemoshow_dispatch_pick_tale;

	actor->base.pointer_enter = nemoshow_dispatch_pointer_enter;
	actor->base.pointer_leave = nemoshow_dispatch_pointer_leave;
	actor->base.pointer_motion = nemoshow_dispatch_pointer_motion;
	actor->base.pointer_axis = nemoshow_dispatch_pointer_axis;
	actor->base.pointer_button = nemoshow_dispatch_pointer_button;

	actor->base.keyboard_enter = nemoshow_dispatch_keyboard_enter;
	actor->base.keyboard_leave = nemoshow_dispatch_keyboard_leave;
	actor->base.keyboard_key = nemoshow_dispatch_keyboard_key;
	actor->base.keyboard_modifiers = nemoshow_dispatch_keyboard_modifiers;

	actor->base.keypad_enter = nemoshow_dispatch_keypad_enter;
	actor->base.keypad_leave = nemoshow_dispatch_keypad_leave;
	actor->base.keypad_key = nemoshow_dispatch_keypad_key;
	actor->base.keypad_modifiers = nemoshow_dispatch_keypad_modifiers;

	actor->base.touch_down = nemoshow_dispatch_touch_down;
	actor->base.touch_up = nemoshow_dispatch_touch_up;
	actor->base.touch_motion = nemoshow_dispatch_touch_motion;
	actor->base.touch_frame = nemoshow_dispatch_touch_frame;

	nemoactor_set_dispatch_resize(actor, nemoshow_dispatch_actor_resize);
	nemoactor_set_dispatch_frame(actor, nemoshow_dispatch_actor_frame);

	scon->tale = nemotale_create_gl();
	nemotale_set_backend(scon->tale,
			nemotale_create_fbo(
				actor->texture,
				actor->base.width,
				actor->base.height));
	nemotale_resize(scon->tale, actor->base.width, actor->base.height);
	nemotale_set_dispatch_event(scon->tale, dispatch);

	show = nemoshow_create();
	nemoshow_set_tale(show, scon->tale);
	nemoshow_set_context(show, scon);

	nemotale_set_userdata(scon->tale, show);

	actor->context = scon->tale;

	scon->shell = shell;
	scon->actor = actor;
	scon->timer = timer;
	scon->width = width;
	scon->height = height;
	scon->show = show;

	return show;

err2:
	nemoactor_destroy(actor);

err1:
	free(scon);

	return NULL;
}

void nemoshow_destroy_on_actor(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoshow_destroy(show);

	nemotale_destroy_gl(scon->tale);

	nemoactor_destroy(scon->actor);

	nemotimer_destroy(scon->timer);

	free(scon);
}
