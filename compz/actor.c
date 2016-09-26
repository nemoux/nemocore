#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <actor.h>
#include <content.h>
#include <event.h>
#include <compz.h>
#include <view.h>
#include <renderer.h>
#include <screen.h>
#include <seat.h>
#include <pointer.h>
#include <keyboard.h>
#include <keypad.h>
#include <touch.h>
#include <timer.h>
#include <nemomisc.h>

static int nemoactor_dispatch_pick_me(struct nemocontent *content, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	return nemoactor_dispatch_pick(actor, x, y);
}

static void nemoactor_dispatch_pointer_enter(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = pointer->id;
	event.serial = 0;

	nemoactor_dispatch_event(actor, NEMOEVENT_POINTER_ENTER_TYPE, &event);
}

static void nemoactor_dispatch_pointer_leave(struct nemopointer *pointer, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = pointer->id;
	event.serial = 0;

	nemoactor_dispatch_event(actor, NEMOEVENT_POINTER_LEAVE_TYPE, &event);
}

static void nemoactor_dispatch_pointer_motion(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, float x, float y)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = pointer->id;
	event.serial = 0;
	event.time = time;
	event.x = x;
	event.y = y;

	nemoactor_dispatch_event(actor, NEMOEVENT_POINTER_MOTION_TYPE, &event);
}

static void nemoactor_dispatch_pointer_axis(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t axis, float value)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = pointer->id;
	event.serial = 0;
	event.time = time;
	event.state = axis;
	event.r = value;

	nemoactor_dispatch_event(actor, NEMOEVENT_POINTER_AXIS_TYPE, &event);
}

static void nemoactor_dispatch_pointer_button(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = pointer->id;
	event.serial = 0;
	event.time = time;
	event.value = button;
	event.state = state;

	nemoactor_dispatch_event(actor, NEMOEVENT_POINTER_BUTTON_TYPE, &event);
}

static void nemoactor_dispatch_keyboard_enter(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = keyboard->id;
	event.serial = 0;

	nemoactor_dispatch_event(actor, NEMOEVENT_KEYBOARD_ENTER_TYPE, &event);
}

static void nemoactor_dispatch_keyboard_leave(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = keyboard->id;
	event.serial = 0;

	nemoactor_dispatch_event(actor, NEMOEVENT_KEYBOARD_LEAVE_TYPE, &event);
}

static void nemoactor_dispatch_keyboard_key(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = keyboard->id;
	event.serial = 0;
	event.time = time;
	event.value = key;
	event.state = state;

	nemoactor_dispatch_event(actor, NEMOEVENT_KEYBOARD_KEY_TYPE, &event);
}

static void nemoactor_dispatch_keyboard_modifiers(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static void nemoactor_dispatch_keypad_enter(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = keypad->id;
	event.serial = 0;

	nemoactor_dispatch_event(actor, NEMOEVENT_KEYBOARD_ENTER_TYPE, &event);
}

static void nemoactor_dispatch_keypad_leave(struct nemokeypad *keypad, struct nemocontent *content)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = keypad->id;
	event.serial = 0;

	nemoactor_dispatch_event(actor, NEMOEVENT_KEYBOARD_LEAVE_TYPE, &event);
}

static void nemoactor_dispatch_keypad_key(struct nemokeypad *keypad, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = keypad->id;
	event.serial = 0;
	event.time = time;
	event.value = key;
	event.state = state;

	nemoactor_dispatch_event(actor, NEMOEVENT_KEYBOARD_KEY_TYPE, &event);
}

static void nemoactor_dispatch_keypad_modifiers(struct nemokeypad *keypad, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static void nemoactor_dispatch_touch_down(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = touchid;
	event.serial = 0;
	event.time = time;
	event.x = x;
	event.y = y;
	event.gx = gx;
	event.gy = gy;

	nemoactor_dispatch_event(actor, NEMOEVENT_TOUCH_DOWN_TYPE, &event);
}

static void nemoactor_dispatch_touch_up(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = touchid;
	event.serial = 0;
	event.time = time;

	nemoactor_dispatch_event(actor, NEMOEVENT_TOUCH_UP_TYPE, &event);
}

static void nemoactor_dispatch_touch_motion(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = touchid;
	event.serial = 0;
	event.time = time;
	event.x = x;
	event.y = y;
	event.gx = gx;
	event.gy = gy;

	nemoactor_dispatch_event(actor, NEMOEVENT_TOUCH_MOTION_TYPE, &event);
}

static void nemoactor_dispatch_touch_pressure(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float p)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);
	struct nemoevent event;

	event.device = touchid;
	event.serial = 0;
	event.time = time;
	event.p = p;

	nemoactor_dispatch_event(actor, NEMOEVENT_TOUCH_PRESSURE_TYPE, &event);
}

static void nemoactor_dispatch_touch_frame(struct touchpoint *tp, struct nemocontent *content)
{
}

static void nemoactor_update_output(struct nemocontent *content, uint32_t node_mask, uint32_t screen_mask)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	if (actor->dispatch_output != NULL)
		actor->dispatch_output(actor, node_mask, screen_mask);

	if (content->node_mask != node_mask) {
		content->dirty = 1;

		pixman_region32_union_rect(&content->damage, &content->damage,
				0, 0, content->width, content->height);
	}

	content->node_mask = node_mask;
	content->screen_mask = screen_mask;
}

static void nemoactor_update_transform(struct nemocontent *content, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	if (actor->dispatch_transform != NULL)
		actor->dispatch_transform(actor, visible, x, y, width, height);
}

static void nemoactor_update_layer(struct nemocontent *content, int visible)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	if (actor->dispatch_layer != NULL)
		actor->dispatch_layer(actor, visible);
}

static void nemoactor_update_fullscreen(struct nemocontent *content, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	if (actor->dispatch_fullscreen != NULL)
		actor->dispatch_fullscreen(actor, id, x, y, width, height);
}

static int nemoactor_read_pixels(struct nemocontent *content, pixman_format_code_t format, void *pixels)
{
}

struct nemoactor *nemoactor_create_pixman(struct nemocompz *compz, int width, int height)
{
	struct nemoactor *actor;

	actor = (struct nemoactor *)malloc(sizeof(struct nemoactor));
	if (actor == NULL)
		return NULL;
	memset(actor, 0, sizeof(struct nemoactor));

	actor->data = malloc(width * height * 4);
	if (actor->data == NULL)
		goto err1;

	actor->view = nemoview_create(compz, &actor->base);
	if (actor->view == NULL)
		goto err2;

	nemoview_put_state(actor->view, NEMOVIEW_CATCH_STATE);

	actor->view->actor = (struct nemoactor *)container_of(actor->view->content, struct nemoactor, base);

	actor->newly_attached = 1;

	actor->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, actor->data, width * 4);

	actor->compz = compz;

	actor->base.width = width;
	actor->base.height = height;

	actor->base.get_viewport_transform = NULL;
	actor->base.get_buffer_scale = NULL;
	actor->base.transform_to_buffer_point = NULL;
	actor->base.transform_to_buffer_rect = NULL;

	actor->base.update_output = nemoactor_update_output;
	actor->base.update_transform = nemoactor_update_transform;
	actor->base.update_layer = nemoactor_update_layer;
	actor->base.update_fullscreen = nemoactor_update_fullscreen;
	actor->base.read_pixels = nemoactor_read_pixels;

	actor->base.pick = nemoactor_dispatch_pick_me;

	actor->base.pointer_enter = nemoactor_dispatch_pointer_enter;
	actor->base.pointer_leave = nemoactor_dispatch_pointer_leave;
	actor->base.pointer_motion = nemoactor_dispatch_pointer_motion;
	actor->base.pointer_axis = nemoactor_dispatch_pointer_axis;
	actor->base.pointer_button = nemoactor_dispatch_pointer_button;

	actor->base.keyboard_enter = nemoactor_dispatch_keyboard_enter;
	actor->base.keyboard_leave = nemoactor_dispatch_keyboard_leave;
	actor->base.keyboard_key = nemoactor_dispatch_keyboard_key;
	actor->base.keyboard_modifiers = nemoactor_dispatch_keyboard_modifiers;

	actor->base.keypad_enter = nemoactor_dispatch_keypad_enter;
	actor->base.keypad_leave = nemoactor_dispatch_keypad_leave;
	actor->base.keypad_key = nemoactor_dispatch_keypad_key;
	actor->base.keypad_modifiers = nemoactor_dispatch_keypad_modifiers;

	actor->base.touch_down = nemoactor_dispatch_touch_down;
	actor->base.touch_up = nemoactor_dispatch_touch_up;
	actor->base.touch_motion = nemoactor_dispatch_touch_motion;
	actor->base.touch_pressure = nemoactor_dispatch_touch_pressure;
	actor->base.touch_frame = nemoactor_dispatch_touch_frame;

	actor->min_width = 0;
	actor->min_height = 0;
	actor->max_width = nemocompz_get_scene_width(compz);
	actor->max_height = nemocompz_get_scene_height(compz);

	actor->scale.ax = 0.5f;
	actor->scale.ay = 0.5f;

	wl_signal_init(&actor->destroy_signal);
	wl_signal_init(&actor->ungrab_signal);

	wl_list_init(&actor->link);
	wl_list_insert(&compz->actor_list, &actor->link);

	wl_list_init(&actor->frame_link);

	nemocontent_prepare(&actor->base, compz->nodemax);

	return actor;

err2:
	free(actor->data);

err1:
	free(actor);

	return NULL;
}

int nemoactor_resize_pixman(struct nemoactor *actor, int width, int height)
{
	if (actor->base.width != width || actor->base.height != height) {
		pixman_image_unref(actor->image);
		free(actor->data);

		actor->image = NULL;

		actor->data = malloc(width * height * 4);
		if (actor->data == NULL)
			return -1;

		actor->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, actor->data, width * 4);

		actor->newly_attached = 1;

		actor->base.width = width;
		actor->base.height = height;

		nemoview_transform_dirty(actor->view);
	}

	return 0;
}

void nemoactor_destroy(struct nemoactor *actor)
{
	wl_signal_emit(&actor->destroy_signal, actor);

	wl_list_remove(&actor->link);

	wl_list_remove(&actor->frame_link);

	nemoview_destroy(actor->view);

	nemocontent_finish(&actor->base);

	if (actor->image != NULL) {
		pixman_image_unref(actor->image);
		free(actor->data);
	}

	if (actor->texture > 0) {
		glDeleteTextures(1, &actor->texture);
	}

	free(actor);
}

struct nemoactor *nemoactor_create_gl(struct nemocompz *compz, int width, int height)
{
	struct nemoactor *actor;

	actor = (struct nemoactor *)malloc(sizeof(struct nemoactor));
	if (actor == NULL)
		return NULL;
	memset(actor, 0, sizeof(struct nemoactor));

	actor->view = nemoview_create(compz, &actor->base);
	if (actor->view == NULL)
		goto err1;

	nemoview_put_state(actor->view, NEMOVIEW_CATCH_STATE);

	actor->view->actor = (struct nemoactor *)container_of(actor->view->content, struct nemoactor, base);

	actor->newly_attached = 1;

	glGenTextures(1, &actor->texture);
	glBindTexture(GL_TEXTURE_2D, actor->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	actor->compz = compz;

	actor->base.width = width;
	actor->base.height = height;

	actor->base.get_viewport_transform = NULL;
	actor->base.get_buffer_scale = NULL;
	actor->base.transform_to_buffer_point = NULL;
	actor->base.transform_to_buffer_rect = NULL;

	actor->base.update_output = nemoactor_update_output;
	actor->base.update_transform = nemoactor_update_transform;
	actor->base.update_layer = nemoactor_update_layer;
	actor->base.update_fullscreen = nemoactor_update_fullscreen;
	actor->base.read_pixels = nemoactor_read_pixels;

	actor->base.pick = nemoactor_dispatch_pick_me;

	actor->base.pointer_enter = nemoactor_dispatch_pointer_enter;
	actor->base.pointer_leave = nemoactor_dispatch_pointer_leave;
	actor->base.pointer_motion = nemoactor_dispatch_pointer_motion;
	actor->base.pointer_axis = nemoactor_dispatch_pointer_axis;
	actor->base.pointer_button = nemoactor_dispatch_pointer_button;

	actor->base.keyboard_enter = nemoactor_dispatch_keyboard_enter;
	actor->base.keyboard_leave = nemoactor_dispatch_keyboard_leave;
	actor->base.keyboard_key = nemoactor_dispatch_keyboard_key;
	actor->base.keyboard_modifiers = nemoactor_dispatch_keyboard_modifiers;

	actor->base.keypad_enter = nemoactor_dispatch_keypad_enter;
	actor->base.keypad_leave = nemoactor_dispatch_keypad_leave;
	actor->base.keypad_key = nemoactor_dispatch_keypad_key;
	actor->base.keypad_modifiers = nemoactor_dispatch_keypad_modifiers;

	actor->base.touch_down = nemoactor_dispatch_touch_down;
	actor->base.touch_up = nemoactor_dispatch_touch_up;
	actor->base.touch_motion = nemoactor_dispatch_touch_motion;
	actor->base.touch_pressure = nemoactor_dispatch_touch_pressure;
	actor->base.touch_frame = nemoactor_dispatch_touch_frame;

	actor->min_width = 0;
	actor->min_height = 0;
	actor->max_width = nemocompz_get_scene_width(compz);
	actor->max_height = nemocompz_get_scene_height(compz);

	actor->scale.ax = 0.5f;
	actor->scale.ay = 0.5f;

	wl_signal_init(&actor->destroy_signal);
	wl_signal_init(&actor->ungrab_signal);

	wl_list_init(&actor->link);
	wl_list_insert(&compz->actor_list, &actor->link);

	wl_list_init(&actor->frame_link);

	nemocontent_prepare(&actor->base, compz->nodemax);

	return actor;

err1:
	free(actor);

	return NULL;
}

int nemoactor_resize_gl(struct nemoactor *actor, int width, int height)
{
	if (actor->base.width != width || actor->base.height != height) {
		glBindTexture(GL_TEXTURE_2D, actor->texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);

		actor->newly_attached = 1;

		actor->base.width = width;
		actor->base.height = height;

		nemoview_transform_dirty(actor->view);
	}

	return 0;
}

void nemoactor_schedule_repaint(struct nemoactor *actor)
{
	struct nemoscreen *screen;

	nemoview_update_transform(actor->view);

	wl_list_for_each(screen, &actor->compz->screen_list, link) {
		if (actor->base.screen_mask & (1 << screen->id))
			nemoscreen_schedule_repaint(screen);
	}
}

void nemoactor_damage_dirty(struct nemoactor *actor)
{
	pixman_region32_union_rect(&actor->base.damage, &actor->base.damage,
			0, 0, actor->base.width, actor->base.height);

	actor->base.dirty = 1;

	nemoactor_schedule_repaint(actor);
}

void nemoactor_damage_below(struct nemoactor *actor)
{
	nemoview_damage_below(actor->view);
}

void nemoactor_damage(struct nemoactor *actor, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_union_rect(&actor->base.damage, &actor->base.damage, x, y, width, height);

	actor->base.dirty = 1;

	nemoactor_schedule_repaint(actor);
}

void nemoactor_damage_region(struct nemoactor *actor, pixman_region32_t *region)
{
	pixman_region32_union(&actor->base.damage, &actor->base.damage, region);

	actor->base.dirty = 1;

	nemoactor_schedule_repaint(actor);
}

void nemoactor_flush_damage(struct nemoactor *actor)
{
	struct rendernode *node;

	if (!actor->base.dirty)
		return;

	wl_list_for_each(node, &actor->compz->render_list, link) {
		if (actor->base.node_mask & (1 << node->id)) {
			if (node->pixman != NULL) {
				struct nemorenderer *renderer = node->pixman;

				if (actor->newly_attached != 0) {
					renderer->attach_actor(renderer, actor);

					actor->newly_attached = 0;
				}

				renderer->flush_actor(renderer, actor);
			}
			if (node->opengl != NULL) {
				struct nemorenderer *renderer = node->opengl;

				if (actor->newly_attached != 0) {
					renderer->attach_actor(renderer, actor);

					actor->newly_attached = 0;
				}

				renderer->flush_actor(renderer, actor);
			}
		}
	}
}

void nemoactor_set_dispatch_event(struct nemoactor *actor, nemoactor_dispatch_event_t dispatch)
{
	actor->dispatch_event = dispatch;
}

void nemoactor_set_dispatch_pick(struct nemoactor *actor, nemoactor_dispatch_pick_t dispatch)
{
	actor->dispatch_pick = dispatch;
}

void nemoactor_set_dispatch_resize(struct nemoactor *actor, nemoactor_dispatch_resize_t dispatch)
{
	actor->dispatch_resize = dispatch;
}

void nemoactor_set_dispatch_output(struct nemoactor *actor, nemoactor_dispatch_output_t dispatch)
{
	actor->dispatch_output = dispatch;
}

void nemoactor_set_dispatch_transform(struct nemoactor *actor, nemoactor_dispatch_transform_t dispatch)
{
	actor->dispatch_transform = dispatch;
}

void nemoactor_set_dispatch_layer(struct nemoactor *actor, nemoactor_dispatch_layer_t dispatch)
{
	actor->dispatch_layer = dispatch;
}

void nemoactor_set_dispatch_fullscreen(struct nemoactor *actor, nemoactor_dispatch_fullscreen_t dispatch)
{
	actor->dispatch_fullscreen = dispatch;
}

void nemoactor_set_dispatch_frame(struct nemoactor *actor, nemoactor_dispatch_frame_t dispatch)
{
	actor->dispatch_frame = dispatch;
}

void nemoactor_set_dispatch_destroy(struct nemoactor *actor, nemoactor_dispatch_destroy_t dispatch)
{
	actor->dispatch_destroy = dispatch;
}

int nemoactor_dispatch_event(struct nemoactor *actor, uint32_t type, struct nemoevent *event)
{
	if (actor->dispatch_event != NULL)
		return actor->dispatch_event(actor, type, event);

	return 0;
}

int nemoactor_dispatch_pick(struct nemoactor *actor, float x, float y)
{
	if (actor->dispatch_pick != NULL)
		return actor->dispatch_pick(actor, x, y);

	return 0;
}

int nemoactor_dispatch_resize(struct nemoactor *actor, int32_t width, int32_t height)
{
	if (actor->dispatch_resize != NULL)
		return actor->dispatch_resize(actor, width, height);

	return 0;
}

void nemoactor_dispatch_output(struct nemoactor *actor, uint32_t node_mask, uint32_t screen_mask)
{
	if (actor->dispatch_output != NULL)
		actor->dispatch_output(actor, node_mask, screen_mask);
}

void nemoactor_dispatch_transform(struct nemoactor *actor, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	if (actor->dispatch_transform != NULL)
		actor->dispatch_transform(actor, visible, x, y, width, height);
}

void nemoactor_dispatch_layer(struct nemoactor *actor, int visible)
{
	if (actor->dispatch_layer != NULL)
		actor->dispatch_layer(actor, visible);
}

void nemoactor_dispatch_fullscreen(struct nemoactor *actor, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	if (actor->dispatch_fullscreen != NULL)
		actor->dispatch_fullscreen(actor, id, x, y, width, height);
}

void nemoactor_dispatch_frame(struct nemoactor *actor)
{
	if (wl_list_empty(&actor->frame_link))
		actor->dispatch_frame(actor, 0);
}

void nemoactor_dispatch_feedback(struct nemoactor *actor)
{
	if (wl_list_empty(&actor->frame_link)) {
		struct nemocompz *compz = actor->compz;

		wl_list_insert(&compz->frame_list, &actor->frame_link);

		nemocompz_dispatch_frame(compz);
	}
}

void nemoactor_terminate_feedback(struct nemoactor *actor)
{
	if (!wl_list_empty(&actor->frame_link)) {
		wl_list_remove(&actor->frame_link);
		wl_list_init(&actor->frame_link);
	}
}

int nemoactor_dispatch_destroy(struct nemoactor *actor)
{
	if (actor->dispatch_destroy != NULL)
		return actor->dispatch_destroy(actor);

	return 0;
}

void nemoactor_set_min_size(struct nemoactor *actor, uint32_t width, uint32_t height)
{
	actor->min_width = width;
	actor->min_height = height;
}

void nemoactor_set_max_size(struct nemoactor *actor, uint32_t width, uint32_t height)
{
	actor->max_width = width;
	actor->max_height = height;
}
