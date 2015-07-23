#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <input.h>
#include <compz.h>
#include <screen.h>
#include <nemoitem.h>
#include <nemomisc.h>

static void nemoinput_handle_screen_destroy(struct wl_listener *listener, void *data)
{
	struct inputnode *node = (struct inputnode *)container_of(listener, struct inputnode, screen_destroy_listener);

	node->screen = NULL;

	wl_list_remove(&node->screen_destroy_listener.link);
	wl_list_init(&node->screen_destroy_listener.link);
}

void nemoinput_set_screen(struct inputnode *node, struct nemoscreen *screen)
{
	if (node->screen_destroy_listener.notify) {
		wl_list_remove(&node->screen_destroy_listener.link);
		node->screen_destroy_listener.notify = NULL;
	}

	if (screen != NULL) {
		node->screen_destroy_listener.notify = nemoinput_handle_screen_destroy;
		wl_signal_add(&screen->destroy_signal, &node->screen_destroy_listener);
	}

	node->screen = screen;
}

void nemoinput_put_screen(struct inputnode *node)
{
	wl_list_remove(&node->screen_destroy_listener.link);
}

int nemoinput_get_config_screen(struct nemocompz *compz, const char *devnode, uint32_t *nodeid, uint32_t *screenid)
{
	const char *value;
	int renderer = 0;
	int index;

	index = nemoitem_get_ifone(compz->configs, "//nemoshell/input", 0, "devnode", devnode);
	if (index < 0)
		return 0;

	value = nemoitem_get_attr(compz->configs, index, "nodeid");
	if (value != NULL && nodeid != NULL) {
		*nodeid = strtoul(value, 0, 10);
	}

	value = nemoitem_get_attr(compz->configs, index, "screenid");
	if (value != NULL && screenid != NULL) {
		*screenid = strtoul(value, 0, 10);
	}

	return 1;
}
