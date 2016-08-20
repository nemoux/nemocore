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
#include <nemoscope.h>
#include <nemomisc.h>

static int nemoshow_dispatch_actor_pick(struct nemoactor *actor, float x, float y)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	float sx, sy;

	x *= tale->viewport.rx;
	y *= tale->viewport.ry;

	if (nemotale_get_node_count(tale) == 0)
		return pixman_region32_contains_point(&tale->input, x, y, NULL);

	return nemotale_pick_node(tale, x, y, &sx, &sy) != NULL;
}

static int nemoshow_dispatch_actor_event(struct nemoactor *actor, uint32_t type, struct nemoevent *event)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);

	if (type & NEMOEVENT_POINTER_ENTER_TYPE) {
		nemotale_push_pointer_enter_event(tale, event->serial, event->device, event->x, event->y);
	} else if (type & NEMOEVENT_POINTER_LEAVE_TYPE) {
		nemotale_push_pointer_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOEVENT_POINTER_MOTION_TYPE) {
		nemotale_push_pointer_motion_event(tale, event->serial, event->device, event->time, event->x, event->y);
	} else if (type & NEMOEVENT_POINTER_BUTTON_TYPE) {
		if (event->state == WL_POINTER_BUTTON_STATE_PRESSED)
			nemotale_push_pointer_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_pointer_up_event(tale, event->serial, event->device, event->time, event->value);
	} else if (type & NEMOEVENT_POINTER_AXIS_TYPE) {
		nemotale_push_pointer_axis_event(tale, event->serial, event->device, event->time, event->state, event->r);
	} else if (type & NEMOEVENT_KEYBOARD_ENTER_TYPE) {
		nemotale_push_keyboard_enter_event(tale, event->serial, event->device);
	} else if (type & NEMOEVENT_KEYBOARD_LEAVE_TYPE) {
		nemotale_push_keyboard_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOEVENT_KEYBOARD_KEY_TYPE) {
		if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
			nemotale_push_keyboard_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_keyboard_up_event(tale, event->serial, event->device, event->time, event->value);
	} else if (type & NEMOEVENT_KEYBOARD_MODIFIERS_TYPE) {
	} else if (type & NEMOEVENT_TOUCH_DOWN_TYPE) {
		nemotale_push_touch_down_event(tale, event->serial, event->device, event->time, event->x, event->y, event->gx, event->gy);
	} else if (type & NEMOEVENT_TOUCH_UP_TYPE) {
		nemotale_push_touch_up_event(tale, event->serial, event->device, event->time);
	} else if (type & NEMOEVENT_TOUCH_MOTION_TYPE) {
		nemotale_push_touch_motion_event(tale, event->serial, event->device, event->time, event->x, event->y, event->gx, event->gy);
	} else if (type & NEMOEVENT_TOUCH_PRESSURE_TYPE) {
		nemotale_push_touch_pressure_event(tale, event->serial, event->device, event->time, event->p);
	} else if (type & NEMOEVENT_STICK_ENTER_TYPE) {
		nemotale_push_stick_enter_event(tale, event->serial, event->device);
	} else if (type & NEMOEVENT_STICK_LEAVE_TYPE) {
		nemotale_push_stick_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOEVENT_STICK_TRANSLATE_TYPE) {
		nemotale_push_stick_translate_event(tale, event->serial, event->device, event->time, event->x, event->y, event->z);
	} else if (type & NEMOEVENT_STICK_ROTATE_TYPE) {
		nemotale_push_stick_rotate_event(tale, event->serial, event->device, event->time, event->x, event->y, event->z);
	} else if (type & NEMOEVENT_STICK_BUTTON_TYPE) {
		if (event->state == WL_POINTER_BUTTON_STATE_PRESSED)
			nemotale_push_stick_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_stick_up_event(tale, event->serial, event->device, event->time, event->value);
	}

	return 0;
}

static int nemoshow_dispatch_actor_resize(struct nemoactor *actor, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct talefbo *fbo = (struct talefbo *)nemotale_get_backend(tale);

	if (show->dispatch_resize != NULL) {
		show->dispatch_resize(show, width, height);
		return 0;
	}

	nemoactor_resize_gl(actor, width, height);

	nemotale_resize_fbo(fbo, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_update_one(show);
	nemoshow_render_one(show);

	nemotale_composite_fbo_full(tale);

	nemoactor_damage_dirty(actor);

	return 0;
}

static void nemoshow_dispatch_actor_frame(struct nemoactor *actor, uint32_t msecs)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct nemocompz *compz = actor->compz;
	pixman_region32_t region;

	pixman_region32_init(&region);

	nemocompz_make_current(compz);

	if (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, msecs);
		nemoshow_destroy_transition(show);

		nemoactor_dispatch_feedback(actor);
	} else {
		nemoactor_terminate_feedback(actor);
	}

	nemoshow_update_one(show);
	nemoshow_render_one(show);

	nemotale_composite_fbo(tale, &region);

	nemoactor_damage_region(actor, &region);

	pixman_region32_fini(&region);
}

static void nemoshow_dispatch_actor_transform(struct nemoactor *actor, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_transform != NULL)
		show->dispatch_transform(show, visible, x, y, width, height);
}

static void nemoshow_dispatch_actor_layer(struct nemoactor *actor, int32_t visible)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_layer != NULL)
		show->dispatch_layer(show, visible);
}

static void nemoshow_dispatch_actor_fullscreen(struct nemoactor *actor, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_fullscreen != NULL)
		show->dispatch_fullscreen(show, id, x, y, width, height);
}

static int nemoshow_dispatch_actor_destroy(struct nemoactor *actor)
{
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (show->dispatch_destroy != NULL)
		return show->dispatch_destroy(show);

	return 0;
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

	if (show->dispatch_event != NULL) {
		show->dispatch_event(show, event);
	} else {
		if (nemotale_dispatch_grab(tale, event) == 0) {
			uint32_t id = nemotale_node_get_id(node);

			if (id != 0) {
				struct showone *one = (struct showone *)nemotale_node_get_data(node);
				struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

				if (canvas->dispatch_event != NULL)
					canvas->dispatch_event(show, one, event);
			}
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

	scon->actor = actor = nemoactor_create_gl(compz, width, height);
	if (actor == NULL)
		goto err1;

	timer = nemotimer_create(compz);
	if (timer == NULL)
		goto err2;
	nemotimer_set_callback(timer, nemoshow_dispatch_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, scon);

	nemoactor_set_dispatch_event(actor, nemoshow_dispatch_actor_event);
	nemoactor_set_dispatch_pick(actor, nemoshow_dispatch_actor_pick);
	nemoactor_set_dispatch_resize(actor, nemoshow_dispatch_actor_resize);
	nemoactor_set_dispatch_frame(actor, nemoshow_dispatch_actor_frame);
	nemoactor_set_dispatch_transform(actor, nemoshow_dispatch_actor_transform);
	nemoactor_set_dispatch_layer(actor, nemoshow_dispatch_actor_layer);
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
	nemoshow_set_size(show, width, height);
	nemoshow_set_context(show, scon);

	nemoview_set_state(scon->actor->view, NEMOVIEW_CLOSE_STATE);

	nemotale_set_userdata(scon->tale, show);

	nemoactor_set_context(actor, scon->tale);

	scon->shell = shell;
	scon->compz = compz;
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

	nemoactor_set_dispatch_pick(scon->actor, NULL);
	nemoactor_set_dispatch_event(scon->actor, NULL);
}

void nemoshow_dispatch_frame(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoactor_dispatch_frame(scon->actor);
}

void nemoshow_dispatch_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoactor_dispatch_resize(scon->actor, width, height);
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

	if (layer == NULL || strcmp(layer, "service") == 0)
		nemoview_attach_layer(actor->view, &shell->service_layer);
	else if (strcmp(layer, "background") == 0)
		nemoview_attach_layer(actor->view, &shell->background_layer);
	else if (strcmp(layer, "underlay") == 0)
		nemoview_attach_layer(actor->view, &shell->underlay_layer);
	else if (strcmp(layer, "overlay") == 0)
		nemoview_attach_layer(actor->view, &shell->overlay_layer);

	nemoview_set_state(actor->view, NEMOVIEW_MAP_STATE);
}

void nemoshow_view_put_layer(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_detach_layer(actor->view);

	nemoview_put_state(actor->view, NEMOVIEW_MAP_STATE);
}

void nemoshow_view_set_fullscreen_type(struct nemoshow *show, const char *type)
{
}

void nemoshow_view_put_fullscreen_type(struct nemoshow *show, const char *type)
{
}

void nemoshow_view_set_fullscreen(struct nemoshow *show, const char *id)
{
}

void nemoshow_view_put_fullscreen(struct nemoshow *show)
{
}

void nemoshow_view_set_parent(struct nemoshow *show, struct nemoview *parent)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_parent(actor->view, parent);
	nemoview_set_state(actor->view, NEMOVIEW_MAP_STATE);
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

void nemoshow_view_set_scale(struct nemoshow *show, float sx, float sy)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_scale(actor->view, sx, sy);
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

void nemoshow_view_set_anchor(struct nemoshow *show, float ax, float ay)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_anchor(actor->view, ax, ay);
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

void nemoshow_view_set_state(struct nemoshow *show, const char *state)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	if (strcmp(state, "catch") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_CATCH_STATE);
	else if (strcmp(state, "pick") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_PICK_STATE);
	else if (strcmp(state, "keypad") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_KEYPAD_STATE);
	else if (strcmp(state, "sound") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_SOUND_STATE);
	else if (strcmp(state, "layer") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_LAYER_STATE);
	else if (strcmp(state, "opaque") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_OPAQUE_STATE);
	else if (strcmp(state, "stage") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_STAGE_STATE);
	else if (strcmp(state, "smooth") == 0)
		nemoview_set_state(actor->view, NEMOVIEW_SMOOTH_STATE);
}

void nemoshow_view_put_state(struct nemoshow *show, const char *state)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	if (strcmp(state, "catch") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_CATCH_STATE);
	else if (strcmp(state, "pick") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_PICK_STATE);
	else if (strcmp(state, "keypad") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_KEYPAD_STATE);
	else if (strcmp(state, "sound") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_SOUND_STATE);
	else if (strcmp(state, "layer") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_LAYER_STATE);
	else if (strcmp(state, "opaque") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_OPAQUE_STATE);
	else if (strcmp(state, "stage") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_STAGE_STATE);
	else if (strcmp(state, "smooth") == 0)
		nemoview_put_state(actor->view, NEMOVIEW_SMOOTH_STATE);
}

void nemoshow_view_set_region(struct nemoshow *show, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_region(actor->view, x, y, width, height);
}

void nemoshow_view_put_region(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_put_region(actor->view);
}

void nemoshow_view_set_scope(struct nemoshow *show, const char *fmt, ...)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;
	va_list vargs;
	char cmd[512];

	va_start(vargs, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, vargs);
	va_end(vargs);

	nemoview_set_scope(actor->view, cmd);
}

void nemoshow_view_put_scope(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_put_scope(actor->view);
}

void nemoshow_view_set_framerate(struct nemoshow *show, uint32_t framerate)
{
}

void nemoshow_view_set_tag(struct nemoshow *show, uint32_t tag)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_tag(actor->view, tag);
}

void nemoshow_view_set_type(struct nemoshow *show, const char *type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;

	nemoview_set_type(actor->view, type);
}

int nemoshow_view_move(struct nemoshow *show, uint64_t device)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoseat *seat = scon->compz->seat;
	struct nemoactor *actor = scon->actor;

	return 0;
}

int nemoshow_view_pick(struct nemoshow *show, uint64_t device0, uint64_t device1, uint32_t type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoseat *seat = scon->compz->seat;
	struct nemoactor *actor = scon->actor;

	return 0;
}

int nemoshow_view_pick_distant(struct nemoshow *show, void *event, uint32_t type)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoseat *seat = scon->compz->seat;
	struct nemoactor *actor = scon->actor;

	return 0;
}

void nemoshow_view_miss(struct nemoshow *show)
{
}

void nemoshow_view_focus(struct nemoshow *show, uint32_t id)
{
}

void nemoshow_view_focus_on(struct nemoshow *show, double x, double y)
{
}

void nemoshow_view_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);
	struct talefbo *fbo = (struct talefbo *)nemotale_get_backend(tale);

	nemoactor_resize_gl(actor, width, height);

	nemotale_resize_fbo(fbo, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_update_one(show);
}

void nemoshow_view_redraw(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);
	struct nemoactor *actor = scon->actor;
	struct nemotale *tale = (struct nemotale *)nemoactor_get_context(actor);

	nemoshow_update_one(show);
	nemoshow_render_one(show);

	nemotale_composite_fbo_full(tale);

	nemoactor_damage_dirty(actor);
}
