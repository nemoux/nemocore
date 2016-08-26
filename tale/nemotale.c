#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <limits.h>

#include <nemotale.h>
#include <talenode.h>
#include <talegl.h>
#include <talepixman.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemomisc.h>

int nemotale_prepare(struct nemotale *tale)
{
	nemosignal_init(&tale->destroy_signal);

	nemolist_init(&tale->ptap_list);
	nemolist_init(&tale->tap_list);
	nemolist_init(&tale->grab_list);

	pixman_region32_init(&tale->damage);

	nemomatrix_init_identity(&tale->transform.matrix);

	tale->viewport.sx = 1.0f;
	tale->viewport.sy = 1.0f;
	tale->viewport.rx = 1.0f;
	tale->viewport.ry = 1.0f;

	nemolist_init(&tale->node_list);

	tale->keyboard.focus = NULL;
	nemolist_init(&tale->keyboard.node_destroy_listener.link);

	tale->long_press_duration = 1500;
	tale->long_press_distance = 50;
	tale->single_click_duration = 700;
	tale->single_click_distance = 30;

	return 0;
}

void nemotale_finish(struct nemotale *tale)
{
	nemosignal_emit(&tale->destroy_signal, tale);

	pixman_region32_fini(&tale->damage);

	nemolist_remove(&tale->keyboard.node_destroy_listener.link);

	nemolist_remove(&tale->ptap_list);
	nemolist_remove(&tale->tap_list);
	nemolist_remove(&tale->grab_list);

	nemolist_remove(&tale->node_list);
}

struct talenode *nemotale_pick_node(struct nemotale *tale, float x, float y, float *sx, float *sy)
{
	struct talenode *node;

	nemolist_for_each_reverse(node, &tale->node_list, link) {
		if (node->id > 0) {
			nemotale_node_transform_from_global(node, x, y, sx, sy);

			if (pixman_region32_contains_point(&node->input, *sx, *sy, NULL))
				return node;
		}
	}

	return NULL;
}

int nemotale_contain_node(struct nemotale *tale, struct talenode *node, float x, float y, float *sx, float *sy)
{
	nemotale_node_transform_from_global(node, x, y, sx, sy);

	if (pixman_region32_contains_point(&node->input, *sx, *sy, NULL))
		return 1;

	return 0;
}

void nemotale_damage_region(struct nemotale *tale, pixman_region32_t *region)
{
	pixman_region32_union(&tale->damage, &tale->damage, region);
}

void nemotale_damage_below(struct nemotale *tale, struct talenode *node)
{
	pixman_region32_union(&tale->damage, &tale->damage, &node->boundingbox);
}

void nemotale_damage_all(struct nemotale *tale)
{
	pixman_region32_union_rect(&tale->damage, &tale->damage, 0, 0, tale->width, tale->height);
}

void nemotale_attach_node(struct nemotale *tale, struct talenode *node)
{
	nemolist_remove(&node->link);
	nemolist_insert_tail(&tale->node_list, &node->link);

	nemotale_damage_below(tale, node);
	nemotale_prepare_node(tale, node);

	node->tale = tale;
}

void nemotale_detach_node(struct talenode *node)
{
	nemolist_remove(&node->link);
	nemolist_init(&node->link);

	if (node->tale != NULL) {
		nemotale_damage_below(node->tale, node);

		node->tale = NULL;
	}
}

void nemotale_above_node(struct nemotale *tale, struct talenode *node, struct talenode *above)
{
	nemolist_remove(&node->link);

	if (above != NULL)
		nemolist_insert(&above->link, &node->link);
	else
		nemolist_insert_tail(&tale->node_list, &node->link);

	nemotale_damage_below(tale, node);

	node->tale = tale;
}

void nemotale_below_node(struct nemotale *tale, struct talenode *node, struct talenode *below)
{
	nemolist_remove(&node->link);

	if (below != NULL)
		nemolist_insert_tail(&below->link, &node->link);
	else
		nemolist_insert(&tale->node_list, &node->link);

	nemotale_damage_below(tale, node);

	node->tale = tale;
}

void nemotale_prepare_node(struct nemotale *tale, struct talenode *node)
{
	if (nemotale_has_gl_context(tale) != 0) {
#ifdef NEMOUX_WITH_OPENGL_PBO
		node->dispatch_flush = nemotale_node_flush_gl_pbo;
		node->dispatch_map = nemotale_node_map_pbo;
		node->dispatch_unmap = nemotale_node_unmap_pbo;
		node->dispatch_copy = nemotale_node_copy_pbo;
#elif NEMOUX_WITH_OPENGL_UNPACK_SUBIMAGE
		node->dispatch_flush = nemotale_node_flush_gl_subimage;
#else
		node->dispatch_flush = nemotale_node_flush_gl;
#endif

		node->dispatch_filter = nemotale_node_filter_gl;

		nemotale_node_prepare_gl(node);
	}
}

void nemotale_clear_node(struct nemotale *tale)
{
	struct talenode *node, *next;

	nemolist_for_each_safe(node, next, &tale->node_list, link) {
		nemolist_remove(&node->link);
		nemolist_init(&node->link);
	}

	nemotale_damage_all(tale);
}

void nemotale_update_node(struct nemotale *tale)
{
	struct talenode *node;

	nemolist_for_each(node, &tale->node_list, link) {
		if (node->transform.dirty != 0)
			nemotale_node_update_transform(node);
	}
}

void nemotale_accumulate_damage(struct nemotale *tale)
{
	struct talenode *node;

	nemolist_for_each(node, &tale->node_list, link) {
		if (node->has_filter != 0 || node->needs_redraw != 0) {
			nemotale_damage_below(tale, node);

			node->needs_redraw = 0;
		} else if (node->dirty != 0) {
			pixman_region32_t damage;

			pixman_region32_init(&damage);

			pixman_region32_intersect(&node->damage, &node->damage, &node->region);

			if (node->transform.enable != 0) {
				pixman_box32_t *extents;

				extents = pixman_region32_extents(&node->damage);
				nemotale_node_update_boundingbox(node,
						extents->x1, extents->y1,
						extents->x2 - extents->x1,
						extents->y2 - extents->y1,
						&damage);
			} else {
				pixman_region32_copy(&damage, &node->damage);
				pixman_region32_translate(&damage,
						node->geometry.x, node->geometry.y);
			}

			pixman_region32_union(&tale->damage, &tale->damage, &damage);

			pixman_region32_fini(&damage);
		}
	}
}

void nemotale_flush_damage(struct nemotale *tale)
{
	struct talenode *node;

	nemolist_for_each(node, &tale->node_list, link) {
		if (node->dirty != 0) {
			pixman_region32_clear(&node->damage);

			node->dirty = 0;
		}
	}

	pixman_region32_clear(&tale->damage);
}

static void nemotale_handle_keyboard_focus_destroy(struct nemolistener *listener, void *data)
{
	struct nemotale *tale = (struct nemotale *)container_of(listener, struct nemotale, keyboard.node_destroy_listener);

	nemolist_remove(&tale->keyboard.node_destroy_listener.link);
	nemolist_init(&tale->keyboard.node_destroy_listener.link);

	tale->keyboard.focus = NULL;
}

void nemotale_set_keyboard_focus(struct nemotale *tale, struct talenode *node)
{
	if (tale->keyboard.focus != NULL) {
		nemolist_remove(&tale->keyboard.node_destroy_listener.link);
		nemolist_init(&tale->keyboard.node_destroy_listener.link);
	}

	tale->keyboard.focus = node;

	if (node != NULL) {
		tale->keyboard.node_destroy_listener.notify = nemotale_handle_keyboard_focus_destroy;
		nemosignal_add(&node->destroy_signal, &tale->keyboard.node_destroy_listener);
	}
}
