#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <layer.h>
#include <view.h>
#include <nemomisc.h>

struct nemolayer *nemolayer_create(struct nemocompz *compz, const char *name)
{
	struct nemolayer *layer;

	layer = (struct nemolayer *)malloc(sizeof(struct nemolayer));
	if (layer == NULL)
		return NULL;
	memset(layer, 0, sizeof(struct nemolayer));

	layer->compz = compz;
	layer->name = strdup(name);

	wl_list_init(&layer->view_list);

	return layer;
}

void nemolayer_destroy(struct nemolayer *layer)
{
	struct nemoview *view, *next;

	wl_list_remove(&layer->link);

	wl_list_for_each_safe(view, next, &layer->view_list, layer_link) {
		nemoview_detach_layer(view);
	}

	free(layer->name);
	free(layer);
}

void nemolayer_attach_below(struct nemolayer *layer, struct nemolayer *below)
{
	if (below != NULL)
		wl_list_insert(below->link.next, &layer->link);
	else
		wl_list_insert(layer->compz->layer_list.prev, &layer->link);
}

void nemolayer_attach_above(struct nemolayer *layer, struct nemolayer *above)
{
	if (above != NULL)
		wl_list_insert(above->link.prev, &layer->link);
	else
		wl_list_insert(&layer->compz->layer_list, &layer->link);
}

void nemolayer_detach(struct nemolayer *layer)
{
	wl_list_remove(&layer->link);
	wl_list_init(&layer->link);
}
