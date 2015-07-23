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

void nemolayer_prepare(struct nemolayer *layer, struct wl_list *below)
{
	wl_list_init(&layer->view_list);

	if (below != NULL)
		wl_list_insert(below, &layer->link);
}

void nemolayer_finish(struct nemolayer *layer)
{
	struct nemoview *view, *next;

	wl_list_for_each_safe(view, next, &layer->view_list, layer_link) {
		nemoview_detach_layer(view);
	}

	wl_list_remove(&layer->link);
	wl_list_init(&layer->link);
}
