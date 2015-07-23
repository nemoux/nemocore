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
#include <actor.h>
#include <view.h>
#include <content.h>
#include <screen.h>
#include <layer.h>
#include <nemomisc.h>

static void pixmanrenderer_finish_actor_content(struct pixmanrenderer *renderer, struct pixmancontent *pmcontent)
{
	wl_list_remove(&pmcontent->actor_destroy_listener.link);
	wl_list_remove(&pmcontent->renderer_destroy_listener.link);

	nemocontent_set_pixman_context(pmcontent->content, renderer->base.node, NULL);

	if (pmcontent->image) {
		pixman_image_unref(pmcontent->image);
	}

	free(pmcontent);
}

static void pixmanrenderer_handle_actor_destroy(struct wl_listener *listener, void *data)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)container_of(listener, struct pixmancontent, actor_destroy_listener);

	pixmanrenderer_finish_actor_content(pmcontent->renderer, pmcontent);
}

static void pixmanrenderer_handle_actor_renderer_destroy(struct wl_listener *listener, void *data)
{
	struct pixmancontent *pmcontent = (struct pixmancontent *)container_of(listener, struct pixmancontent, renderer_destroy_listener);

	pixmanrenderer_finish_actor_content(pmcontent->renderer, pmcontent);
}

static struct pixmancontent *pixmanrenderer_prepare_actor_content(struct pixmanrenderer *renderer, struct nemoactor *actor)
{
	struct pixmancontent *pmcontent;

	pmcontent = (struct pixmancontent *)malloc(sizeof(struct pixmancontent));
	if (pmcontent == NULL)
		return NULL;
	memset(pmcontent, 0, sizeof(struct pixmancontent));

	pmcontent->renderer = renderer;
	pmcontent->content = &actor->base;

	pmcontent->actor_destroy_listener.notify = pixmanrenderer_handle_actor_destroy;
	wl_signal_add(&actor->destroy_signal, &pmcontent->actor_destroy_listener);

	pmcontent->renderer_destroy_listener.notify = pixmanrenderer_handle_actor_renderer_destroy;
	wl_signal_add(&renderer->destroy_signal, &pmcontent->renderer_destroy_listener);

	return pmcontent;
}

void pixmanrenderer_attach_actor(struct nemorenderer *base, struct nemoactor *actor)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmancontent *pmcontent;

	pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(&actor->base, base->node);
	if (pmcontent == NULL) {
		pmcontent = pixmanrenderer_prepare_actor_content(renderer, actor);
		nemocontent_set_pixman_context(&actor->base, base->node, pmcontent);
	}

	if (pmcontent->image != NULL) {
		pixman_image_unref(pmcontent->image);
		pmcontent->image = NULL;
	}

	if (actor->image == NULL)
		return;

	pmcontent->image = pixman_image_ref(actor->image);
}

void pixmanrenderer_flush_actor(struct nemorenderer *base, struct nemoactor *actor)
{
}

int pixmanrenderer_read_actor(struct nemorenderer *base, struct nemoactor *actor, pixman_format_code_t format, void *pixels)
{
	struct pixmanrenderer *renderer = (struct pixmanrenderer *)container_of(base, struct pixmanrenderer, base);
	struct pixmancontent *pmcontent;
	uint32_t width, height;
	pixman_image_t *image;

	pmcontent = (struct pixmancontent *)nemocontent_get_pixman_context(&actor->base, base->node);
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
