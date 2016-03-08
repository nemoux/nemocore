#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showring.h>
#include <nemomisc.h>

struct showone *nemoshow_ring_create(int32_t width, int32_t height)
{
	struct showone *one;
	struct showone *icon;
	struct showring *ring;

	ring = (struct showring *)malloc(sizeof(struct showring));
	if (ring == NULL)
		return NULL;
	memset(ring, 0, sizeof(struct showring));

	one = nemoshow_item_create(NEMOSHOW_CONTAINER_ITEM);
	if (one == NULL)
		goto err1;

	nemoshow_item_set_base_width(one, width);
	nemoshow_item_set_base_height(one, height);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);

	ring->icon0 = icon = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(one, icon);
	nemoshow_item_set_x(icon, 0.0f);
	nemoshow_item_set_y(icon, 0.0f);
	nemoshow_item_set_width(icon, width);
	nemoshow_item_set_height(icon, height);
	nemoshow_item_set_fill_color(icon, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(icon, NEMOSHOW_SOLID_SMALL_BLUR);
	nemoshow_item_pivot(icon, width / 2.0f, height / 2.0f);

	ring->icon1 = icon = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_one_attach(one, icon);
	nemoshow_item_set_x(icon, 0.0f);
	nemoshow_item_set_y(icon, 0.0f);
	nemoshow_item_set_width(icon, width);
	nemoshow_item_set_height(icon, height);
	nemoshow_item_set_fill_color(icon, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_filter(icon, NEMOSHOW_SOLID_SMALL_BLUR);
	nemoshow_item_pivot(icon, width / 2.0f, height / 2.0f);

	nemoshow_one_set_dispatch_dattr(one, nemoshow_ring_set_dattr);
	nemoshow_one_set_dispatch_sattr(one, nemoshow_ring_set_sattr);
	nemoshow_one_set_dispatch_destroy(one, nemoshow_ring_destroy);
	nemoshow_one_set_context(one, ring);

	nemoshow_one_setd(one, "inner_scale", 0.0f);
	nemoshow_one_setd(one, "outer_scale", 0.0f);
	nemoshow_one_setd(one, "inner_rotate", 0.0f);
	nemoshow_one_setd(one, "outer_rotate", 0.0f);
	nemoshow_one_setd(one, "inner_alpha", 0.0f);
	nemoshow_one_setd(one, "outer_alpha", 0.0f);

	return one;

err1:
	free(ring);

	return NULL;
}

void nemoshow_ring_destroy(struct showone *one)
{
	struct showring *ring = (struct showring *)nemoshow_one_get_context(one);

	nemoshow_item_destroy(one);

	free(ring);
}

int nemoshow_ring_set_dattr(struct showone *one, const char *attr, double value)
{
	struct showring *ring = (struct showring *)nemoshow_one_get_context(one);

	if (strcmp(attr, "inner_scale") == 0) {
		nemoshow_item_scale(ring->icon0, value, value);
	} else if (strcmp(attr, "outer_scale") == 0) {
		nemoshow_item_scale(ring->icon1, value, value);
	} else if (strcmp(attr, "inner_rotate") == 0) {
		nemoshow_item_rotate(ring->icon0, value);
	} else if (strcmp(attr, "outer_rotate") == 0) {
		nemoshow_item_rotate(ring->icon1, value);
	} else if (strcmp(attr, "inner_alpha") == 0) {
		nemoshow_item_set_alpha(ring->icon0, value);
	} else if (strcmp(attr, "outer_alpha") == 0) {
		nemoshow_item_set_alpha(ring->icon1, value);
	}

	return 0;
}

int nemoshow_ring_set_sattr(struct showone *one, const char *attr, const char *value)
{
	struct showring *ring = (struct showring *)nemoshow_one_get_context(one);

	if (strcmp(attr, "inner_icon") == 0) {
		nemoshow_item_load_svg(ring->icon0, value);
	} else if (strcmp(attr, "outer_icon") == 0) {
		nemoshow_item_load_svg(ring->icon1, value);
	}

	return 0;
}
