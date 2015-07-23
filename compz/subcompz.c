#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <compz.h>
#include <canvas.h>
#include <subcompz.h>
#include <subcanvas.h>
#include <canvas.h>

static void subcompositor_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void subcompositor_get_subsurface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, struct wl_resource *parent_resource)
{
	nemosubcanvas_create(client, resource, id, surface_resource, parent_resource);
}

static const struct wl_subcompositor_interface subcompositor_implementation = {
	subcompositor_destroy,
	subcompositor_get_subsurface
};

int nemosubcompz_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_subcompositor_interface, 1, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &subcompositor_implementation, compz, NULL);

	return 0;
}
