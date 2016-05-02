#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <limits.h>

#include <nemotale.h>
#include <talenode.h>
#include <nemolist.h>
#include <nemolistener.h>
#include <nemomisc.h>

int nemotale_prepare(struct nemotale *tale)
{
	char *env;

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

	tale->nodes = (struct talenode **)malloc(sizeof(struct talenode *) * 8);
	tale->nnodes = 0;
	tale->snodes = 8;

	tale->keyboard.focus = NULL;
	nemolist_init(&tale->keyboard.node_destroy_listener.link);

	env = getenv("NEMOTALE_LONG_PRESS_DURATION");
	if (env != NULL)
		tale->long_press_duration = strtoul(env, NULL, 10);
	else
		tale->long_press_duration = 1500;

	env = getenv("NEMOTALE_LONG_PRESS_DISTANCE");
	if (env != NULL)
		tale->long_press_distance = strtoul(env, NULL, 10);
	else
		tale->long_press_distance = 50;

	env = getenv("NEMOTALE_SINGLE_CLICK_DURATION");
	if (env != NULL)
		tale->single_click_duration = strtoul(env, NULL, 10);
	else
		tale->single_click_duration = 700;

	env = getenv("NEMOTALE_SINGLE_CLICK_DISTANCE");
	if (env != NULL)
		tale->single_click_distance = strtoul(env, NULL, 10);
	else
		tale->single_click_distance = 30;

	env = getenv("NEMOTALE_CLOSE_WIDTH");
	if (env != NULL)
		tale->close_width = strtoul(env, NULL, 10);
	else
		tale->close_width = 250;

	env = getenv("NEMOTALE_CLOSE_HEIGHT");
	if (env != NULL)
		tale->close_height = strtoul(env, NULL, 10);
	else
		tale->close_height = 250;

	env = getenv("NEMOTALE_MINIMUM_WIDTH");
	if (env != NULL)
		tale->minimum_width = strtoul(env, NULL, 10);
	else
		tale->minimum_width = 200;

	env = getenv("NEMOTALE_MINIMUM_HEIGHT");
	if (env != NULL)
		tale->minimum_height = strtoul(env, NULL, 10);
	else
		tale->minimum_height = 200;

	env = getenv("NEMOTALE_MAXIMUM_WIDTH");
	if (env != NULL)
		tale->maximum_width = strtoul(env, NULL, 10);
	else
		tale->maximum_width = UINT32_MAX;

	env = getenv("NEMOTALE_MAXIMUM_HEIGHT");
	if (env != NULL)
		tale->maximum_height = strtoul(env, NULL, 10);
	else
		tale->maximum_height = UINT32_MAX;

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

	free(tale->nodes);
}

struct talenode *nemotale_pick_node(struct nemotale *tale, float x, float y, float *sx, float *sy)
{
	struct talenode *node;
	int i;

	for (i = tale->nnodes - 1; i >= 0; i--) {
		node = tale->nodes[i];

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
	if (node->tale != NULL && node->tale != tale)
		nemotale_detach_node(node);

	NEMOBOX_APPEND(tale->nodes, tale->snodes, tale->nnodes, node);

	nemotale_damage_below(tale, node);

	node->tale = tale;
}

void nemotale_detach_node(struct talenode *node)
{
	if (node->tale != NULL) {
		struct nemotale *tale = node->tale;
		int i;

		for (i = 0; i < tale->nnodes; i++) {
			if (tale->nodes[i] == node) {
				NEMOBOX_REMOVE(tale->nodes, tale->nnodes, i);
				break;
			}
		}

		nemotale_damage_below(tale, node);

		node->tale = NULL;
	}
}

void nemotale_above_node(struct nemotale *tale, struct talenode *node, struct talenode *above)
{
	int i;

	if (node->tale != NULL) {
		for (i = 0; i < node->tale->nnodes; i++) {
			if (node->tale->nodes[i] == node) {
				NEMOBOX_REMOVE(node->tale->nodes, node->tale->nnodes, i);
				break;
			}
		}
	}

	if (above != NULL) {
		for (i = 0; i < tale->nnodes; i++) {
			if (tale->nodes[i] == above) {
				NEMOBOX_PUSH(tale->nodes, tale->snodes, tale->nnodes, i + 1, node);

				break;
			}
		}
	} else {
		NEMOBOX_PUSH(tale->nodes, tale->snodes, tale->nnodes, tale->nnodes, node);
	}

	nemotale_damage_below(tale, node);

	node->tale = tale;
}

void nemotale_below_node(struct nemotale *tale, struct talenode *node, struct talenode *below)
{
	int i;

	if (node->tale != NULL) {
		for (i = 0; i < node->tale->nnodes; i++) {
			if (node->tale->nodes[i] == node) {
				NEMOBOX_REMOVE(node->tale->nodes, node->tale->nnodes, i);
				break;
			}
		}
	}

	if (below != NULL) {
		for (i = 0; i < tale->nnodes; i++) {
			if (tale->nodes[i] == below) {
				NEMOBOX_PUSH(tale->nodes, tale->snodes, tale->nnodes, i, node);

				break;
			}
		}
	} else {
		NEMOBOX_PUSH(tale->nodes, tale->snodes, tale->nnodes, 0, node);
	}

	nemotale_damage_below(tale, node);

	node->tale = tale;
}

void nemotale_clear_node(struct nemotale *tale)
{
	tale->nnodes = 0;

	nemotale_damage_all(tale);
}

void nemotale_update_node(struct nemotale *tale)
{
	struct talenode *node;
	int i;

	for (i = 0; i < tale->nnodes; i++) {
		node = tale->nodes[i];

		if (node->transform.dirty != 0) {
			nemotale_damage_below(tale, node);
			nemotale_node_transform_update(node);
			nemotale_damage_below(tale, node);

			node->transform.dirty = 0;
		}
	}
}

void nemotale_accumulate_damage(struct nemotale *tale)
{
	struct talenode *node;
	int i;

	for (i = 0; i < tale->nnodes; i++) {
		node = tale->nodes[i];

		if (node->has_filter != 0) {
			nemotale_damage_below(tale, node);
		} else if (node->dirty != 0) {
			pixman_region32_t damage;

			pixman_region32_init(&damage);

			pixman_region32_intersect(&node->damage, &node->damage, &node->region);

			if (node->transform.enable != 0) {
				pixman_box32_t *extents;

				extents = pixman_region32_extents(&node->damage);
				nemotale_node_boundingbox_update(node,
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
	int i;

	for (i = 0; i < tale->nnodes; i++) {
		node = tale->nodes[i];

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
