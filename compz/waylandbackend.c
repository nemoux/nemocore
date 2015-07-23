#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-client.h>
#include <wayland-server.h>

#include <waylandbackend.h>
#include <waylandnode.h>
#include <compz.h>
#include <nemomisc.h>

struct nemobackend *waylandbackend_create(struct nemocompz *compz)
{
	struct waylandbackend *wl;
	struct waylandnode *node;
	char *name;

	name = getenv("WAYLAND_DISPLAY");
	if (name == NULL)
		return NULL;

	wl = (struct waylandbackend *)malloc(sizeof(struct waylandbackend));
	if (wl == NULL)
		return NULL;
	memset(wl, 0, sizeof(struct waylandbackend));

	wl->base.destroy = waylandbackend_destroy;

	wl->compz = compz;

	node = wayland_create_node(compz, name);
	if (node == NULL)
		goto err1;

	wl_list_insert(&compz->backend_list, &wl->base.link);

	return &wl->base;

err1:
	free(wl);

	return NULL;
}

void waylandbackend_destroy(struct nemobackend *base)
{
	struct waylandbackend *wl = (struct waylandbackend *)container_of(base, struct waylandbackend, base);
	struct rendernode *node, *next;

	wl_list_remove(&base->link);

	wl_list_for_each_safe(node, next, &wl->compz->render_list, link) {
		wayland_destroy_node((struct waylandnode *)container_of(node, struct waylandnode, base));
	}

	free(wl);
}
