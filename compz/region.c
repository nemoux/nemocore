#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <wayland-server.h>

#include <region.h>

static void nemoregion_unbind_region(struct wl_resource *resource)
{
	struct nemoregion *region = (struct nemoregion *)wl_resource_get_user_data(resource);

	nemoregion_destroy(region);
}

static void region_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void region_add(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemoregion *region = (struct nemoregion *)wl_resource_get_user_data(resource);

	pixman_region32_union_rect(&region->region, &region->region, x, y, width, height);
}

static void region_subtract(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemoregion *region = (struct nemoregion *)wl_resource_get_user_data(resource);
	pixman_region32_t rect;

	pixman_region32_init_rect(&rect, x, y, width, height);
	pixman_region32_subtract(&region->region, &region->region, &rect);
	pixman_region32_fini(&rect);
}

static const struct wl_region_interface region_implementation = {
	region_destroy,
	region_add,
	region_subtract
};

struct nemoregion *nemoregion_create(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id)
{
	struct nemoregion *region;

	region = (struct nemoregion *)malloc(sizeof(struct nemoregion));
	if (region == NULL) {
		wl_client_post_no_memory(client);
		return NULL;
	}
	memset(region, 0, sizeof(struct nemoregion));

	region->resource = wl_resource_create(client, &wl_region_interface, 2, id);
	if (region->resource == NULL) {
		wl_client_post_no_memory(client);
		goto err1;
	}

	wl_resource_set_implementation(region->resource, &region_implementation, region, nemoregion_unbind_region);

	pixman_region32_init(&region->region);

	return region;

err1:
	free(region);

	return NULL;
}

void nemoregion_destroy(struct nemoregion *region)
{
	pixman_region32_fini(&region->region);

	free(region);
}
