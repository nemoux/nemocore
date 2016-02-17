#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <shell.h>
#include <compz.h>
#include <view.h>
#include <actor.h>
#include <screen.h>
#include <keyboard.h>
#include <keypad.h>
#include <pointer.h>
#include <touch.h>
#include <seat.h>
#include <move.h>
#include <pick.h>
#include <timer.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <nemomisc.h>

static int nemoshow_dispatch_pick_tale(struct nemocontent *content, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;
	float sx, sy;

	x *= tale->viewport.rx;
	y *= tale->viewport.ry;

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

static void nemoshow_dispatch_touch_down(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_down_event(tale, 0, touchid, time, x, y, gx, gy);
}

static void nemoshow_dispatch_touch_up(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_up_event(tale, 0, touchid, time);
}

static void nemoshow_dispatch_touch_motion(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemotale *tale = (struct nemotale *)actor->context;

	nemotale_push_touch_motion_event(tale, 0, touchid, time, x, y, gx, gy);
}

static void nemoshow_dispatch_touch_frame(struct touchpoint *tp, struct nemocontent *content)
{
}

static int nemoshow_dispatch_actor_resize(struct nemoactor *actor, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct talefbo *fbo = (struct talefbo *)nemotale_get_backend(tale);

	if (width < nemotale_get_minimum_width(tale) || height < nemotale_get_minimum_height(tale)) {
		nemoactor_dispatch_destroy(actor);
		return 0;
	}

	nemoactor_resize_gl(actor, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_render_one(show);

	nemotale_resize_fbo(fbo, width, height);

	nemotale_composite_fbo_full(tale);

	nemoactor_damage_dirty(actor);

	return 0;
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
		nemoactor_dispatch_feedback(actor);
	} else if (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, msecs);
		nemoshow_destroy_transition(show);

		nemoactor_dispatch_feedback(actor);
	} else {
		nemoactor_terminate_feedback(actor);
	}

	nemoshow_render_one(show);

	nemotale_composite_fbo(tale, &region);

	nemoactor_damage_region(actor, &region);

	pixman_region32_fini(&region);
}

static void nemoshow_dispatch_actor_transform(struct nemoactor *actor, int32_t visible)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_transform != NULL)
		show->dispatch_transform(show, visible);
}

static void nemoshow_dispatch_actor_fullscreen(struct nemoactor *actor, int32_t active, int32_t opaque)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_fullscreen != NULL)
		show->dispatch_fullscreen(show, active, opaque);
}

static void nemoshow_dispatch_actor_destroy(struct nemoactor *actor)
{
	struct nemotale *tale = (struct nemotale *)actor->context;
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_destroy != NULL)
		show->dispatch_destroy(show);
}

static void nemoshow_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct showcontext *scon = (struct showcontext *)data;

	nemotimer_set_timeout(timer, 500);

	nemotale_push_timer_event(scon->tale, time_current_msecs());
}

static void nemoshow_dispatch_tale_event(struct nemotale *tale, struct talenode *node, struct taleevent *event)
{
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	uint32_t id = nemotale_node_get_id(node);

	if (nemotale_dispatch_grab(tale, event) == 0) {
		if (id == 0) {
			if (show->dispatch_event != NULL)
				show->dispatch_event(show, event);
		} else {
			struct showone *one = (struct showone *)nemotale_node_get_data(node);
			struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

			if (canvas->dispatch_event != NULL)
				canvas->dispatch_event(show, one, event);
		}
	}
}

struct nemoshow *nemoshow_create_view(struct nemoshell *shell, int32_t width, int32_t height)
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
	nemotimer_set_timeout(timer, 500);
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
	nemoactor_set_dispatch_transform(actor, nemoshow_dispatch_actor_transform);
	nemoactor_set_dispatch_fullscreen(actor, nemoshow_dispatch_actor_fullscreen);
	nemoactor_set_dispatch_destroy(actor, nemoshow_dispatch_actor_destroy);

	scon->tale = nemotale_create_gl();
	nemotale_set_backend(scon->tale,
			nemotale_create_fbo(
				actor->texture,
				actor->base.width,
				actor->base.height));
	nemotale_resize(scon->tale, actor->base.width, actor->base.height);
	nemotale_set_dispatch_event(scon->tale, nemoshow_dispatch_tale_event);

	show = nemoshow_create();
	nemoshow_set_tale(show, scon->tale);
	nemoshow_set_context(show, scon);

	nemotale_set_userdata(scon->tale, show);

	actor->context = scon->tale;

	scon->shell = shell;
	scon->compz = compz;
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

void nemoshow_destroy_view(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemotimer_destroy(scon->timer);

	nemoshow_destroy(scon->show);

	nemotale_destroy_gl(scon->tale);

	nemoactor_destroy(scon->actor);

	free(scon);
}

static void nemoshow_dispatch_destroy_view(void *data)
{
	struct nemoshow *show = (struct nemoshow *)data;

	nemoshow_destroy_view(show);
}

void nemoshow_destroy_view_on_idle(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemocompz_dispatch_idle(scon->compz, nemoshow_dispatch_destroy_view, show);
}

void nemoshow_revoke_view(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	actor->base.pick = NULL;

	actor->base.pointer_enter = NULL;
	actor->base.pointer_leave = NULL;
	actor->base.pointer_motion = NULL;
	actor->base.pointer_axis = NULL;
	actor->base.pointer_button = NULL;

	actor->base.keyboard_enter = NULL;
	actor->base.keyboard_leave = NULL;
	actor->base.keyboard_key = NULL;
	actor->base.keyboard_modifiers = NULL;

	actor->base.keypad_enter = NULL;
	actor->base.keypad_leave = NULL;
	actor->base.keypad_key = NULL;
	actor->base.keypad_modifiers = NULL;

	actor->base.touch_down = NULL;
	actor->base.touch_up = NULL;
	actor->base.touch_motion = NULL;
	actor->base.touch_frame = NULL;
}

void nemoshow_dispatch_frame(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoactor_dispatch_frame(scon->actor);
}

void nemoshow_dispatch_feedback(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoactor_dispatch_feedback(scon->actor);
}

void nemoshow_terminate_feedback(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoactor_terminate_feedback(scon->actor);
}

void nemoshow_view_set_layer(struct nemoshow *show, const char *layer)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoshell *shell = scon->shell;
	struct nemoactor *actor = scon->actor;

	if (strcmp(layer, "background") == 0)
		nemoview_attach_layer(actor->view, &shell->background_layer);
	else if (strcmp(layer, "underlay") == 0)
		nemoview_attach_layer(actor->view, &shell->underlay_layer);
	else if (strcmp(layer, "overlay") == 0)
		nemoview_attach_layer(actor->view, &shell->overlay_layer);
	else if (strcmp(layer, "service") == 0)
		nemoview_attach_layer(actor->view, &shell->service_layer);

	nemoview_set_state(actor->view, NEMO_VIEW_MAPPED_STATE);
}

void nemoshow_view_put_layer(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_detach_layer(actor->view);

	nemoview_put_state(actor->view, NEMO_VIEW_MAPPED_STATE);
}

void nemoshow_view_set_position(struct nemoshow *show, float x, float y)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_position(actor->view, x, y);
}

void nemoshow_view_set_rotation(struct nemoshow *show, float r)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_rotation(actor->view, r);
}

void nemoshow_view_set_pivot(struct nemoshow *show, float px, float py)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_pivot(actor->view, px, py);
}

void nemoshow_view_put_pivot(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_put_pivot(actor->view);
}

void nemoshow_view_set_flag(struct nemoshow *show, float fx, float fy)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_flag(actor->view, fx, fy);
}

void nemoshow_view_set_opaque(struct nemoshow *show, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;
}

void nemoshow_view_set_min_size(struct nemoshow *show, float width, float height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoactor_set_min_size(actor, width, height);
}

void nemoshow_view_set_max_size(struct nemoshow *show, float width, float height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoactor_set_max_size(actor, width, height);
}

void nemoshow_view_set_input(struct nemoshow *show, const char *type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	if (strcmp(type, "touch") == 0)
		nemoview_set_input_type(actor->view, NEMO_VIEW_INPUT_TOUCH);
	else
		nemoview_set_input_type(actor->view, NEMO_VIEW_INPUT_NORMAL);
}

void nemoshow_view_set_sound(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_state(actor->view, NEMO_VIEW_SOUND_STATE);
}

void nemoshow_view_put_sound(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_put_state(actor->view, NEMO_VIEW_SOUND_STATE);
}

int nemoshow_view_move(struct nemoshow *show, uint64_t device)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoseat *seat = scon->compz->seat;
	struct nemoactor *actor = scon->actor;
	struct touchpoint *tp;

	tp = nemoseat_get_touchpoint_by_id(seat, device);
	if (tp != NULL) {
		nemoshell_move_actor_by_touchpoint(scon->shell, tp, actor);

		return 1;
	}

	return 0;
}

int nemoshow_view_pick(struct nemoshow *show, uint64_t device0, uint64_t device1, uint32_t type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoseat *seat = scon->compz->seat;
	struct nemoactor *actor = scon->actor;
	struct touchpoint *tp0, *tp1;

	tp0 = nemoseat_get_touchpoint_by_id(seat, device0);
	tp1 = nemoseat_get_touchpoint_by_id(seat, device1);
	if (tp0 != NULL && tp1 != NULL) {
		uint32_t ptype = 0x0;

		if (type & NEMOSHOW_VIEW_PICK_ROTATE_TYPE)
			ptype |= (1 << NEMO_SURFACE_PICK_TYPE_ROTATE);
		if (type & NEMOSHOW_VIEW_PICK_SCALE_TYPE)
			ptype |= (1 << NEMO_SURFACE_PICK_TYPE_SCALE);
		if (type & NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE)
			ptype |= (1 << NEMO_SURFACE_PICK_TYPE_MOVE);

		nemoshell_pick_actor_by_touchpoint(scon->shell, tp0, tp1, ptype, actor);

		return 1;
	}

	return 0;
}

int nemoshow_view_pick_distant(struct nemoshow *show, void *event, uint32_t type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoseat *seat = scon->compz->seat;
	struct nemoactor *actor = scon->actor;
	struct touchpoint *tp0, *tp1;
	uint64_t device0, device1;

	nemotale_event_get_distant_taps_devices(show->tale, event, &device0, &device1);

	tp0 = nemoseat_get_touchpoint_by_id(seat, device0);
	tp1 = nemoseat_get_touchpoint_by_id(seat, device1);
	if (tp0 != NULL && tp1 != NULL) {
		uint32_t ptype = 0x0;

		if (type & NEMOSHOW_VIEW_PICK_ROTATE_TYPE)
			ptype |= (1 << NEMO_SURFACE_PICK_TYPE_ROTATE);
		if (type & NEMOSHOW_VIEW_PICK_SCALE_TYPE)
			ptype |= (1 << NEMO_SURFACE_PICK_TYPE_SCALE);
		if (type & NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE)
			ptype |= (1 << NEMO_SURFACE_PICK_TYPE_MOVE);

		nemoshell_pick_actor_by_touchpoint(scon->shell, tp0, tp1, ptype, actor);

		return 1;
	}

	return 0;
}
