#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <pixmanprivate.h>

#include <pixmanrenderer.h>
#include <renderer.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <content.h>
#include <screen.h>
#include <layer.h>
#include <nemomisc.h>
#include <nemolog.h>

static void pixmanrenderer_finish_canvas_content(struct pixmanrenderer *renderer, struct pixmancontent *pmcontent)
{
	wl_list_remove(&pmcontent->canvas_destroy_listener.link);
	wl_list_remove(&pmcontent->renderer_destroy_listener.link);

	if (pmcontent->buffer_destroy_listener.notify) {
		wl_list_remove(&pmcontent->buffer_destroy_listener.link);
	}

	nemocontent_set_pixman_context(pmcontent->content, renderer->base.node, NULL);

	if (pmcontent->image) {
		pixman_image_unref(pmcontent->image);
	}

	nemobuffer_reference(&pmcontent->buffer_reference, NULL);

	free(pmcontent);
}

static void pixmanrenderer_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)container_of(listener, struct pixmancontent, canvas_destroy_listener);

	pixmanrenderer_finish_canvas_content(pmcontent->renderer, pmcontent);
}

static void pixmanrenderer_handle_renderer_destroy(struct wl_listener *listener, void *data)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)container_of(listener, struct pixmancontent, renderer_destroy_listener);

	pixmanrenderer_finish_canvas_content(pmcontent->renderer, pmcontent);
}

static struct pixmancontent *pixmanrenderer_prepare_canvas_content(struct pixmanrenderer *renderer, struct nemocanvas *canvas)
{
	struct pixmancontent *pmcontent;

	pmcontent = (struct pixmancontent *)malloc(sizeof(struct pixmancontent));
	if (pmcontent == NULL)
		return NULL;
	memset(pmcontent, 0, sizeof(struct pixmancontent));

	pmcontent->renderer = renderer;
	pmcontent->content = &canvas->base;

	pmcontent->canvas_destroy_listener.notify = pixmanrenderer_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &pmcontent->canvas_destroy_listener);

	pmcontent->renderer_destroy_listener.notify = pixmanrenderer_handle_renderer_destroy;
	wl_signal_add(&renderer->destroy_signal, &pmcontent->renderer_destroy_listener);

	return pmcontent;
}

static void pixmanrenderer_handle_buffer_destroy(struct wl_listener *listener, void *data)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)container_of(listener, struct pixmancontent, buffer_destroy_listener);

	if (pmcontent->image) {
		pixman_image_unref(pmcontent->image);
		pmcontent->image = NULL;
	}

	pmcontent->buffer_destroy_listener.notify = NULL;
}

void pixmanrenderer_prepare_buffer(struct nemorenderer *base, struct nemobuffer *buffer)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct wl_shm_buffer *shmbuffer;

	shmbuffer = wl_shm_buffer_get(buffer->resource);
	if (shmbuffer == NULL) {
		nemolog_error("PIXMANRENDERER", "pixman renderer supports only shm buffers\n");
		return;
	}

	buffer->shmbuffer = shmbuffer;
	buffer->width = wl_shm_buffer_get_width(shmbuffer);
	buffer->height = wl_shm_buffer_get_height(shmbuffer);
}

void pixmanrenderer_attach_canvas(struct nemorenderer *base, struct nemocanvas *canvas)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmancontent *pmcontent;
	struct nemobuffer *buffer = canvas->buffer_reference.buffer;
	struct wl_shm_buffer *shmbuffer;
	pixman_format_code_t format;

	pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(&canvas->base, base->node);
	if (pmcontent == NULL) {
		pmcontent = pixmanrenderer_prepare_canvas_content(renderer, canvas);
		nemocontent_set_pixman_context(&canvas->base, base->node, pmcontent);
	}

	nemobuffer_reference(&pmcontent->buffer_reference, buffer);

	if (pmcontent->buffer_destroy_listener.notify) {
		wl_list_remove(&pmcontent->buffer_destroy_listener.link);
		pmcontent->buffer_destroy_listener.notify = NULL;
	}

	if (pmcontent->image) {
		pixman_image_unref(pmcontent->image);
		pmcontent->image = NULL;
	}

	if (buffer == NULL)
		return;

	shmbuffer = wl_shm_buffer_get(buffer->resource);
	if (shmbuffer == NULL) {
		nemolog_error("PIXMANRENDERER", "pixman renderer supports only shm buffers\n");
		nemobuffer_reference(&pmcontent->buffer_reference, NULL);
		return;
	}

	switch (wl_shm_buffer_get_format(shmbuffer)) {
		case WL_SHM_FORMAT_XRGB8888:
			format = PIXMAN_x8r8g8b8;
			break;

		case WL_SHM_FORMAT_ARGB8888:
			format = PIXMAN_a8r8g8b8;
			break;

		case WL_SHM_FORMAT_RGB565:
			format = PIXMAN_r5g6b5;
			break;

		default:
			nemolog_error("PIXMANRENDERER", "unsupported shm buffer formats\n");
			nemobuffer_reference(&pmcontent->buffer_reference, NULL);
			return;
	}

	pmcontent->image = pixman_image_create_bits(format,
			buffer->width, buffer->height,
			wl_shm_buffer_get_data(shmbuffer),
			wl_shm_buffer_get_stride(shmbuffer));

	pmcontent->buffer_destroy_listener.notify = pixmanrenderer_handle_buffer_destroy;
	wl_signal_add(&buffer->destroy_signal, &pmcontent->buffer_destroy_listener);
}

void pixmanrenderer_flush_canvas(struct nemorenderer *base, struct nemocanvas *canvas)
{
}

int pixmanrenderer_read_canvas(struct nemorenderer *base, struct nemocanvas *canvas, pixman_format_code_t format, void *pixels)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmancontent *pmcontent;
	uint32_t width, height;
	pixman_image_t *image;

	pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(&canvas->base, base->node);
	if (pmcontent == NULL || pmcontent->image == NULL)
		return -1;

	width = pixman_image_get_width(pmcontent->image);
	height = pixman_image_get_height(pmcontent->image);

	image = pixman_image_create_bits(format, width, height, pixels, (PIXMAN_FORMAT_BPP(format) / 8) * width);
	pixman_image_composite32(PIXMAN_OP_SRC,
			pmcontent->image,
			NULL,
			image,
			0, 0,
			0, 0,
			0, 0,
			width, height);
	pixman_image_unref(image);

	return 0;
}

void *pixmanrenderer_get_canvas_buffer(struct nemorenderer *base, struct nemocanvas *canvas)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(&canvas->base, base->node);

	if (pmcontent == NULL)
		return NULL;

	return pmcontent->buffer_reference.buffer;
}
