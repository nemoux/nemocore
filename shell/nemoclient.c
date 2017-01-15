#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-nemo-client-server-protocol.h>

#include <compz.h>
#include <shell.h>
#include <nemoclient.h>
#include <nemomisc.h>

static void nemo_client_alive(struct wl_client *client, struct wl_resource *resource, uint32_t timeout)
{
	struct nemoshell *shell = (struct nemoshell *)wl_resource_get_user_data(resource);

	if (shell->alive_client != NULL) {
		pid_t pid;

		wl_client_get_credentials(client, &pid, NULL, NULL);

		shell->alive_client(shell->userdata, pid, timeout);
	}
}

static const struct nemo_client_interface nemo_client_implementation = {
	nemo_client_alive
};

static void nemoclient_unbind(struct wl_resource *resource)
{
}

int nemoclient_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &nemo_client_interface, version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_implementation(resource, &nemo_client_implementation, shell, nemoclient_unbind);

	return 0;
}
