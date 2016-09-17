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

	pixman_region32_init(&tale->damage);

	nemomatrix_init_identity(&tale->transform.matrix);

	nemolist_init(&tale->node_list);

	return 0;
}

void nemotale_finish(struct nemotale *tale)
{
	nemosignal_emit(&tale->destroy_signal, tale);

	pixman_region32_fini(&tale->damage);

	nemolist_remove(&tale->node_list);
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
		if (node->dirty >= 0x2) {
			nemotale_damage_below(tale, node);
		} else if (node->dirty >= 0x1) {
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
