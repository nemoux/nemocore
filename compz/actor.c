#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <actor.h>
#include <content.h>
#include <compz.h>
#include <view.h>
#include <renderer.h>
#include <screen.h>
#include <seat.h>
#include <pointer.h>
#include <keyboard.h>
#include <nemomisc.h>

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

static void nemoactor_update_transform(struct nemocontent *content, int visible)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	if (actor->dispatch_transform != NULL)
		actor->dispatch_transform(actor, visible);
}

static void nemoactor_update_fullscreen(struct nemocontent *content, int active, int opaque)
{
	struct nemoactor *actor = (struct nemoactor *)container_of(content, struct nemoactor, base);

	if (actor->dispatch_fullscreen != NULL)
		actor->dispatch_fullscreen(actor, active, opaque);
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

	nemoview_put_state(actor->view, NEMO_VIEW_CATCHABLE_STATE);

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
	actor->base.update_fullscreen = nemoactor_update_fullscreen;
	actor->base.read_pixels = nemoactor_read_pixels;

	actor->dispatch_resize = NULL;
	actor->dispatch_output = NULL;
	actor->dispatch_frame = NULL;

	actor->min_width = 0;
	actor->min_height = 0;
	actor->max_width = nemocompz_get_scene_width(compz);
	actor->max_height = nemocompz_get_scene_height(compz);

	actor->ax = 0.5f;
	actor->ay = 0.5f;

	wl_signal_init(&actor->destroy_signal);
	wl_signal_init(&actor->ungrab_signal);
	wl_signal_init(&actor->endgrab_signal);

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

#ifdef NEMOUX_WITH_EGL
	if (actor->texture > 0) {
		glDeleteTextures(1, &actor->texture);
	}
#endif

	free(actor);
}

#ifdef NEMOUX_WITH_EGL
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

	nemoview_put_state(actor->view, NEMO_VIEW_CATCHABLE_STATE);

	actor->view->actor = (struct nemoactor *)container_of(actor->view->content, struct nemoactor, base);

	actor->newly_attached = 1;

	glGenTextures(1, &actor->texture);
	glBindTexture(GL_TEXTURE_2D, actor->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
	actor->base.update_fullscreen = nemoactor_update_fullscreen;
	actor->base.read_pixels = nemoactor_read_pixels;

	actor->dispatch_resize = NULL;
	actor->dispatch_output = NULL;
	actor->dispatch_frame = NULL;

	actor->min_width = 0;
	actor->min_height = 0;
	actor->max_width = nemocompz_get_scene_width(compz);
	actor->max_height = nemocompz_get_scene_height(compz);

	actor->ax = 0.5f;
	actor->ay = 0.5f;

	wl_signal_init(&actor->destroy_signal);
	wl_signal_init(&actor->ungrab_signal);
	wl_signal_init(&actor->endgrab_signal);

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
#endif

void nemoactor_schedule_repaint(struct nemoactor *actor)
{
	struct nemoscreen *screen;

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

	actor->base.dirty = 0;

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

	pixman_region32_clear(&actor->base.damage);
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

void nemoactor_set_dispatch_fullscreen(struct nemoactor *actor, nemoactor_dispatch_fullscreen_t dispatch)
{
	actor->dispatch_fullscreen = dispatch;
}

void nemoactor_set_dispatch_frame(struct nemoactor *actor, nemoactor_dispatch_frame_t dispatch)
{
	actor->dispatch_frame = dispatch;
}

int nemoactor_dispatch_resize(struct nemoactor *actor, int32_t width, int32_t height, int32_t fixed)
{
	if (actor->dispatch_resize != NULL)
		return actor->dispatch_resize(actor, width, height, fixed);

	return 0;
}

void nemoactor_dispatch_output(struct nemoactor *actor, uint32_t node_mask, uint32_t screen_mask)
{
	if (actor->dispatch_output != NULL)
		actor->dispatch_output(actor, node_mask, screen_mask);
}

void nemoactor_dispatch_transform(struct nemoactor *actor, int visible)
{
	if (actor->dispatch_transform != NULL)
		actor->dispatch_transform(actor, visible);
}

void nemoactor_dispatch_fullscreen(struct nemoactor *actor, int active, int opaque)
{
	if (actor->dispatch_fullscreen != NULL)
		actor->dispatch_fullscreen(actor, active, opaque);
}

void nemoactor_dispatch_frame(struct nemoactor *actor)
{
	if (wl_list_empty(&actor->frame_link)) {
		actor->dispatch_frame(actor, 0);
	}
}

void nemoactor_feedback(struct nemoactor *actor)
{
	if (wl_list_empty(&actor->frame_link)) {
		struct nemocompz *compz = actor->compz;

		wl_list_insert(&compz->frame_list, &actor->frame_link);

		nemocompz_dispatch_frame(compz);
	}
}

void nemoactor_feedback_done(struct nemoactor *actor)
{
	wl_list_remove(&actor->frame_link);
	wl_list_init(&actor->frame_link);
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
