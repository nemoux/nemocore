#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

struct nemotale *nemotale_create_pixman(void)
{
	struct nemotale *tale;

	tale = (struct nemotale *)malloc(sizeof(struct nemotale));
	if (tale == NULL)
		return NULL;
	memset(tale, 0, sizeof(struct nemotale));

	nemotale_prepare(tale);

	tale->pmcontext = (struct nemopmtale *)malloc(sizeof(struct nemopmtale));
	if (tale->pmcontext == NULL)
		goto err1;
	memset(tale->pmcontext, 0, sizeof(struct nemopmtale));

	return tale;

err1:
	free(tale);

	return NULL;
}

void nemotale_destroy_pixman(struct nemotale *tale)
{
	nemotale_finish(tale);

	free(tale->pmcontext);
	free(tale);
}

int nemotale_attach_pixman(struct nemotale *tale, void *data, int32_t width, int32_t height, int32_t stride)
{
	struct nemopmtale *context = (struct nemopmtale *)tale->pmcontext;

	context->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, data, stride);
	context->data = data;

	tale->width = width;
	tale->height = height;
	tale->stride = stride;

	pixman_region32_init_rect(&tale->region, 0, 0, width, height);

	return 0;
}

void nemotale_detach_pixman(struct nemotale *tale)
{
	struct nemopmtale *context = (struct nemopmtale *)tale->pmcontext;

	if (context->data != NULL) {
		pixman_image_unref(context->image);

		context->image = NULL;
		context->data = NULL;
	}
}

static inline void nemotale_repaint_region(struct nemotale *tale, struct talenode *node, pixman_region32_t *repaint, pixman_region32_t *region, pixman_op_t op)
{
	struct nemopmtale *context = (struct nemopmtale *)tale->pmcontext;
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;

	if (region == NULL) {
		pixman_image_set_clip_region32(context->image, repaint);

		pixman_image_composite32(op,
				pcontext->image,
				NULL,
				context->image,
				0, 0,
				0, 0,
				0, 0,
				tale->width,
				tale->height);

		pixman_image_set_clip_region32(context->image, NULL);
	} else {
		pixman_region32_t final_region;
		float x, y;

		pixman_region32_init(&final_region);
		pixman_region32_copy(&final_region, region);

		if (node->transform.enable != 0) {
			nemotale_node_transform_to_global(node, 0.0f, 0.0f, &x, &y);
			pixman_region32_translate(&final_region, (int)x, (int)y);
		} else {
			pixman_region32_translate(&final_region, node->geometry.x, node->geometry.y);
		}

		pixman_region32_intersect(&final_region, &final_region, repaint);

		pixman_image_set_clip_region32(context->image, &final_region);

		pixman_image_composite32(op,
				pcontext->image,
				NULL,
				context->image,
				0, 0,
				0, 0,
				0, 0,
				tale->width,
				tale->height);

		pixman_image_set_clip_region32(context->image, NULL);

		pixman_region32_fini(&final_region);
	}
}

static void nemotale_repaint_node(struct nemotale *tale, struct talenode *node)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;
	pixman_region32_t repaint;

	pixman_region32_init(&repaint);
	pixman_region32_intersect(&repaint, &node->boundingbox, &tale->damage);

	if (pixman_region32_not_empty(&repaint)) {
		pixman_transform_t transform;

		pixman_transform_init_identity(&transform);

		if (node->transform.enable != 0) {
			pixman_transform_t matrix = {{{
				pixman_double_to_fixed(node->transform.matrix.d[0]),
					pixman_double_to_fixed(node->transform.matrix.d[4]),
					pixman_double_to_fixed(node->transform.matrix.d[12])
			}, {
				pixman_double_to_fixed(node->transform.matrix.d[1]),
					pixman_double_to_fixed(node->transform.matrix.d[5]),
					pixman_double_to_fixed(node->transform.matrix.d[13])
			}, {
				pixman_double_to_fixed(node->transform.matrix.d[3]),
					pixman_double_to_fixed(node->transform.matrix.d[7]),
					pixman_double_to_fixed(node->transform.matrix.d[15])
			}}};

			pixman_transform_invert(&matrix, &matrix);
			pixman_transform_multiply(&transform, &matrix, &transform);
		} else {
			pixman_transform_translate(&transform, NULL,
					pixman_double_to_fixed((double)-node->geometry.x),
					pixman_double_to_fixed((double)-node->geometry.y));
		}

		pixman_image_set_transform(pcontext->image, &transform);

		if (node->transform.enable != 0)
			pixman_image_set_filter(pcontext->image, PIXMAN_FILTER_BILINEAR, NULL, 0);
		else
			pixman_image_set_filter(pcontext->image, PIXMAN_FILTER_NEAREST, NULL, 0);

		if (node->transform.enable != 0) {
			nemotale_repaint_region(tale, node, &repaint, NULL, PIXMAN_OP_OVER);
		} else {
			if (pixman_region32_not_empty(&node->opaque)) {
				nemotale_repaint_region(tale, node, &repaint, &node->opaque, PIXMAN_OP_SRC);
			}
			if (pixman_region32_not_empty(&node->blend)) {
				nemotale_repaint_region(tale, node, &repaint, &node->blend, PIXMAN_OP_OVER);
			}
		}
	}

	pixman_region32_fini(&repaint);
}

int nemotale_composite_pixman(struct nemotale *tale, pixman_region32_t *region)
{
	struct talenode *node;

	nemotale_update_node(tale);
	nemotale_accumulate_damage(tale);

	nemolist_for_each(node, &tale->node_list, link) {
		nemotale_repaint_node(tale, node);
	}

	if (region != NULL)
		pixman_region32_union(region, region, &tale->damage);

	nemotale_flush_damage(tale);

	return 0;
}

int nemotale_composite_pixman_full(struct nemotale *tale)
{
	struct talenode *node;

	nemotale_update_node(tale);
	nemotale_damage_all(tale);

	nemolist_for_each(node, &tale->node_list, link) {
		nemotale_repaint_node(tale, node);
	}

	return 0;
}

static void nemotale_node_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct talepmnode *context = (struct talepmnode *)container_of(listener, struct talepmnode, destroy_listener);

	nemolist_remove(&context->destroy_listener.link);

	pixman_image_unref(context->image);
	free(context->data);

	free(context);
}

struct talenode *nemotale_node_create_pixman(int32_t width, int32_t height)
{
	struct talenode *node;
	struct talepmnode *pcontext;

	node = (struct talenode *)malloc(sizeof(struct talenode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct talenode));

	node->pmcontext = pcontext = (struct talepmnode *)malloc(sizeof(struct talepmnode));
	if (pcontext == NULL)
		goto err1;
	memset(pcontext, 0, sizeof(struct talepmnode));

	pcontext->data = malloc(width * height * 4);
	if (pcontext->data == NULL)
		goto err2;
	memset(pcontext->data, 0, width * height * 4);

	pcontext->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, pcontext->data, width * 4);
	pcontext->needs_free = 1;

	nemotale_node_prepare(node);

	node->dirty = 0x2;
	node->needs_flush = 1;
	node->needs_filter = 1;
	node->needs_full_upload = 1;
	node->geometry.width = width;
	node->geometry.height = height;

	node->dispatch_resize = nemotale_node_resize_pixman;
	node->dispatch_map = nemotale_node_map_pixman;
	node->dispatch_unmap = nemotale_node_unmap_pixman;

	pixman_region32_init_rect(&node->blend, 0, 0, width, height);
	pixman_region32_init_rect(&node->region, 0, 0, width, height);
	pixman_region32_init_rect(&node->damage, 0, 0, width, height);

	pcontext->destroy_listener.notify = nemotale_node_handle_destroy_signal;
	nemosignal_add(&node->destroy_signal, &pcontext->destroy_listener);

	return node;

err2:
	free(node->pmcontext);

err1:
	free(node);

	return NULL;
}

int nemotale_node_attach_pixman(struct talenode *node, void *data, int32_t width, int32_t height)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;

	node->dirty = 0x2;
	node->needs_flush = 1;
	node->needs_filter = 1;
	node->needs_full_upload = 1;
	node->geometry.width = width;
	node->geometry.height = height;
	node->transform.dirty = 1;

	pixman_region32_init_rect(&node->blend, 0, 0, width, height);
	pixman_region32_init_rect(&node->region, 0, 0, width, height);

	if (pcontext->needs_free != 0)
		free(pcontext->data);
	if (pcontext->image != NULL)
		pixman_image_unref(pcontext->image);

	pcontext->data = data;
	pcontext->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, pcontext->data, width * 4);

	pcontext->needs_free = 0;

	return 0;
}

void nemotale_node_detach_pixman(struct talenode *node)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;

	if (pcontext->needs_free != 0)
		free(pcontext->data);
	if (pcontext->image != NULL)
		pixman_image_unref(pcontext->image);

	pcontext->data = NULL;
	pcontext->image = NULL;
	pcontext->needs_free = 0;
}

void nemotale_node_fill_pixman(struct talenode *node, double r, double g, double b, double a)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;
	pixman_image_t *mask;
	pixman_color_t color;

	color.red = r * 0xffff;
	color.green = g * 0xffff;
	color.blue = b * 0xffff;
	color.alpha = a * 0xffff;

	mask = pixman_image_create_solid_fill(&color);

	pixman_image_composite32(PIXMAN_OP_SRC,
			mask,
			NULL,
			pcontext->image,
			0, 0, 0, 0, 0, 0,
			node->geometry.width, node->geometry.height);

	pixman_image_unref(mask);
}

int nemotale_node_resize_pixman(struct talenode *node, int32_t width, int32_t height)
{
	if (node->geometry.width != width || node->geometry.height != height) {
		struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;

		node->dirty = 0x2;
		node->needs_flush = 1;
		node->needs_filter = 1;
		node->needs_full_upload = 1;
		node->geometry.width = width;
		node->geometry.height = height;
		node->transform.dirty = 1;

		pixman_region32_init_rect(&node->blend, 0, 0, width, height);
		pixman_region32_init_rect(&node->region, 0, 0, width, height);
		pixman_region32_init_rect(&node->damage, 0, 0, width, height);

		if (pcontext->needs_free != 0)
			free(pcontext->data);
		if (pcontext->image != NULL)
			pixman_image_unref(pcontext->image);

		pcontext->data = malloc(width * height * 4);
		memset(pcontext->data, 0, width * height * 4);

		pcontext->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, pcontext->data, width * 4);
		pcontext->needs_free = 1;
	}

	return 0;
}

void *nemotale_node_map_pixman(struct talenode *node)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;

	return pcontext->data;
}

int nemotale_node_unmap_pixman(struct talenode *node)
{
	return 0;
}
