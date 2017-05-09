#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pixmanprivate.h>

#include <pixmanrenderer.h>
#include <pixmancanvas.h>
#include <renderer.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <content.h>
#include <screen.h>
#include <layer.h>
#include <waylandhelper.h>
#include <nemomisc.h>

static int pixmanrenderer_read_pixels(struct nemorenderer *base, struct nemoscreen *screen, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct pixmansurface *surface = (struct pixmansurface *)screen->pcontext;
	pixman_transform_t transform;
	pixman_image_t *image;

	if (surface->screen_image == NULL)
		return -1;

	image = pixman_image_create_bits(format, width, height, pixels, (PIXMAN_FORMAT_BPP(format) / 8) * width);

	pixman_transform_init_translate(&transform,
			pixman_int_to_fixed(x),
			pixman_int_to_fixed(y - pixman_image_get_height(surface->screen_image)));
	pixman_transform_scale(&transform,
			NULL,
			pixman_fixed_1,
			pixman_fixed_minus_1);

	pixman_image_set_transform(surface->screen_image, &transform);

	pixman_image_composite32(PIXMAN_OP_SRC,
			surface->screen_image,
			NULL,
			image,
			0, 0,
			0, 0,
			0, 0,
			pixman_image_get_width(surface->screen_image),
			pixman_image_get_height(surface->screen_image));

	pixman_image_set_transform(surface->screen_image, NULL);

	pixman_image_unref(image);

	return 0;
}

static void pixmanrenderer_copy_to_screen_buffer(struct nemoscreen *screen, struct pixmansurface *surface, pixman_region32_t *region)
{
	pixman_region32_t screen_region;

	pixman_region32_init(&screen_region);
	pixman_region32_copy(&screen_region, region);

	pixman_region32_translate(&screen_region, -screen->x, -screen->y);
	wayland_transform_region(screen->width, screen->height, WL_OUTPUT_TRANSFORM_NORMAL, 1, &screen_region, &screen_region);

	pixman_image_set_clip_region32(surface->screen_image, &screen_region);

	pixman_image_composite32(PIXMAN_OP_SRC,
			surface->shadow_image,
			NULL,
			surface->screen_image,
			0, 0,
			0, 0,
			0, 0,
			pixman_image_get_width(surface->screen_image),
			pixman_image_get_height(surface->screen_image));

	pixman_image_set_clip_region32(surface->screen_image, NULL);
}

static void pixmanrenderer_draw_region(struct pixmanrenderer *renderer, struct nemoview *view, struct nemoscreen *screen, pixman_region32_t *repaint, pixman_region32_t *region, pixman_op_t pixop)
{
	struct pixmansurface *surface = (struct pixmansurface *)screen->pcontext;
	struct pixmancontent *pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(view->content, renderer->base.node);
	pixman_transform_t transform;
	pixman_region32_t final_region;
	pixman_image_t *maskimage = NULL;
	pixman_color_t mask = { 0 };
	float x, y;

	pixman_region32_init(&final_region);
	if (region) {
		pixman_region32_copy(&final_region, region);

		if (!view->transform.enable) {
			pixman_region32_translate(&final_region, view->geometry.x, view->geometry.y);
		} else {
			nemoview_transform_to_global_nocheck(view, 0, 0, &x, &y);
			pixman_region32_translate(&final_region, (int)x, (int)y);
		}

		pixman_region32_intersect(&final_region, &final_region, repaint);
	} else {
		pixman_region32_copy(&final_region, repaint);
	}

	pixman_region32_translate(&final_region, -screen->x, -screen->y);
	wayland_transform_region(screen->width, screen->height, WL_OUTPUT_TRANSFORM_NORMAL, 1, &final_region, &final_region);

	pixman_image_set_clip_region32(surface->shadow_image, &final_region);

	pixman_transform_init_identity(&transform);
	pixman_transform_scale(&transform, NULL,
			pixman_double_to_fixed((double)1.0f),
			pixman_double_to_fixed((double)1.0f));

	pixman_transform_translate(&transform, NULL,
			pixman_double_to_fixed(screen->x),
			pixman_double_to_fixed(screen->y));

	if (view->transform.enable) {
		pixman_transform_t matrix = {{{
			pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 0)),
				pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 4)),
				pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 12))
		}, {
			pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 1)),
				pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 5)),
				pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 13))
		}, {
			pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 3)),
				pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 7)),
				pixman_double_to_fixed(nemomatrix_get_float(&view->transform.matrix, 15))
		}}};

		pixman_transform_invert(&matrix, &matrix);
		pixman_transform_multiply(&transform, &matrix, &transform);
	} else {
		pixman_transform_translate(&transform, NULL,
				pixman_double_to_fixed((double)-view->geometry.x),
				pixman_double_to_fixed((double)-view->geometry.y));
	}

	nemocontent_get_viewport_transform(view->content, &transform);

	pixman_image_set_transform(pmcontent->image, &transform);

	if (view->transform.enable != 0 ||
			screen->transform.enable != 0 ||
			nemocontent_get_buffer_scale(view->content) != 1)
		pixman_image_set_filter(pmcontent->image, PIXMAN_FILTER_BILINEAR, NULL, 0);
	else
		pixman_image_set_filter(pmcontent->image, PIXMAN_FILTER_NEAREST, NULL, 0);

	if (pmcontent->buffer_reference.buffer)
		wl_shm_buffer_begin_access(pmcontent->buffer_reference.buffer->shmbuffer);

	if (view->alpha < 1.0f) {
		mask.alpha = view->alpha * 0xffff;
		maskimage = pixman_image_create_solid_fill(&mask);
	}

	pixman_image_composite32(pixop,
			pmcontent->image,
			maskimage,
			surface->shadow_image,
			0, 0,
			0, 0,
			0, 0,
			pixman_image_get_width(surface->shadow_image),
			pixman_image_get_height(surface->shadow_image));

	if (maskimage != NULL)
		pixman_image_unref(maskimage);

	if (pmcontent->buffer_reference.buffer)
		wl_shm_buffer_end_access(pmcontent->buffer_reference.buffer->shmbuffer);

	pixman_image_set_transform(pmcontent->image, NULL);

	pixman_image_set_clip_region32(surface->shadow_image, NULL);

	pixman_region32_fini(&final_region);
}

static void pixmanrenderer_draw_view(struct pixmanrenderer *renderer, struct nemoview *view, struct nemoscreen *screen, pixman_region32_t *damage)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(view->content, renderer->base.node);
	pixman_region32_t repaint;
	pixman_region32_t blend;

	if (pmcontent == NULL || pmcontent->image == NULL)
		return;

	pixman_region32_init(&repaint);
	pixman_region32_intersect(&repaint, &view->transform.boundingbox, damage);
	pixman_region32_subtract(&repaint, &repaint, &view->clip);

	if (!pixman_region32_not_empty(&repaint))
		goto out;

	if ((view->alpha != 1.0f) || (view->transform.enable && nemomatrix_has_translate_only(&view->transform.matrix) == 0)) {
		pixmanrenderer_draw_region(renderer, view, screen, &repaint, NULL, PIXMAN_OP_OVER);
	} else {
		pixman_region32_init_rect(&blend, 0, 0, view->content->width, view->content->height);
		pixman_region32_subtract(&blend, &blend, &view->content->opaque);

		if (pixman_region32_not_empty(&view->content->opaque)) {
			pixmanrenderer_draw_region(renderer, view, screen, &repaint, &view->content->opaque, PIXMAN_OP_SRC);
		}

		if (pixman_region32_not_empty(&blend)) {
			pixmanrenderer_draw_region(renderer, view, screen, &repaint, &blend, PIXMAN_OP_OVER);
		}

		pixman_region32_fini(&blend);
	}

out:
	pixman_region32_fini(&repaint);
}

static void pixmanrenderer_repaint_views(struct pixmanrenderer *renderer, struct nemoscreen *screen, pixman_region32_t *damage)
{
	struct nemocompz *compz = screen->compz;
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each_reverse(layer, &compz->layer_list, link) {
		wl_list_for_each_reverse(view, &layer->view_list, layer_link) {
			pixmanrenderer_draw_view(renderer, view, screen, damage);

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each_reverse(child, &view->children_list, children_link) {
					pixmanrenderer_draw_view(renderer, child, screen, damage);
				}
			}
		}
	}
}

static void pixmanrenderer_repaint_screen(struct nemorenderer *base, struct nemoscreen *screen, pixman_region32_t *screen_damage)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmansurface *surface = (struct pixmansurface *)screen->pcontext;

	if (surface->screen_image == NULL)
		return;

	pixmanrenderer_repaint_views(renderer, screen, screen_damage);
	pixmanrenderer_copy_to_screen_buffer(screen, surface, screen_damage);
}

struct nemorenderer *pixmanrenderer_create(struct rendernode *node)
{
	struct nemocompz *compz = node->compz;
	struct pixmanrenderer *renderer;

	renderer = (struct pixmanrenderer *)malloc(sizeof(struct pixmanrenderer));
	if (renderer == NULL)
		return NULL;
	memset(renderer, 0, sizeof(struct pixmanrenderer));

	renderer->base.node = node;
	renderer->base.read_pixels = pixmanrenderer_read_pixels;
	renderer->base.repaint_screen = pixmanrenderer_repaint_screen;
	renderer->base.prepare_buffer = pixmanrenderer_prepare_buffer;
	renderer->base.attach_canvas = pixmanrenderer_attach_canvas;
	renderer->base.flush_canvas = pixmanrenderer_flush_canvas;
	renderer->base.read_canvas = pixmanrenderer_read_canvas;
	renderer->base.get_canvas_buffer = pixmanrenderer_get_canvas_buffer;
	renderer->base.destroy = pixmanrenderer_destroy;
	renderer->base.make_current = NULL;

	if (compz->renderer == NULL)
		compz->renderer = &renderer->base;

	wl_display_add_shm_format(node->compz->display, WL_SHM_FORMAT_RGB565);

	wl_signal_init(&renderer->destroy_signal);

	return &renderer->base;
}

void pixmanrenderer_destroy(struct nemorenderer *base)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);

	wl_signal_emit(&renderer->destroy_signal, renderer);

	free(renderer);
}

int pixmanrenderer_prepare_screen(struct nemorenderer *base, struct nemoscreen *screen)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmansurface *surface;
	int width, height;

	surface = (struct pixmansurface *)malloc(sizeof(struct pixmansurface));
	if (surface == NULL)
		return -1;
	memset(surface, 0, sizeof(struct pixmansurface));

	width = screen->current_mode->width;
	height = screen->current_mode->height;

	surface->shadow_buffer = malloc(width * height * 4);
	if (surface->shadow_buffer == NULL)
		goto err1;

	surface->shadow_image = pixman_image_create_bits(PIXMAN_x8r8g8b8,
			width, height,
			surface->shadow_buffer, width * 4);
	if (surface->shadow_image == NULL)
		goto err2;

	screen->pcontext = (void *)surface;

	return 0;

err2:
	free(surface->shadow_buffer);

err1:
	free(surface);

	return -1;
}

void pixmanrenderer_finish_screen(struct nemorenderer *base, struct nemoscreen *screen)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmansurface *surface = (struct pixmansurface *)screen->pcontext;

	pixman_image_unref(surface->shadow_image);

	if (surface->screen_image != NULL)
		pixman_image_unref(surface->screen_image);

	free(surface);
}

void pixmanrenderer_set_screen_buffer(struct nemorenderer *base, struct nemoscreen *screen, pixman_image_t *buffer)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmansurface *surface = (struct pixmansurface *)screen->pcontext;

	if (surface->screen_image != NULL)
		pixman_image_unref(surface->screen_image);

	surface->screen_image = buffer;
	if (surface->screen_image != NULL) {
		screen->read_format = pixman_image_get_format(surface->screen_image);
		pixman_image_ref(surface->screen_image);
	}
}
