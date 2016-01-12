#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <tuiobackend.h>
#include <tuionode.h>
#include <compz.h>
#include <nemoitem.h>
#include <nemomisc.h>

struct nemobackend *tuiobackend_create(struct nemocompz *compz, int index)
{
	struct tuiobackend *tuio;
	struct tuionode *node;
	const char *value;
	int protocol, port, max;
	int i;

	tuio = (struct tuiobackend *)malloc(sizeof(struct tuiobackend));
	if (tuio == NULL)
		return NULL;
	memset(tuio, 0, sizeof(struct tuiobackend));

	tuio->base.destroy = tuiobackend_destroy;

	tuio->compz = compz;

	for (i = 0;
			(i = nemoitem_get(compz->configs, "//nemoshell/tuio", i)) >= 0;
			(i++)) {
		value = nemoitem_get_attr(compz->configs, i, "protocol");
		if (value == NULL || strcmp(value, "osc") != 0)
			protocol = NEMO_TUIO_XML_PROTOCOL;
		else
			protocol = NEMO_TUIO_OSC_PROTOCOL;

		port = nemoitem_get_iattr(compz->configs, i, "port", 3333);
		max = nemoitem_get_iattr(compz->configs, i, "max", 16);

		node = tuio_create_node(compz, protocol, port, max);
		if (node == NULL)
			break;
	}

	wl_list_insert(&compz->backend_list, &tuio->base.link);

	return &tuio->base;
}

void tuiobackend_destroy(struct nemobackend *base)
{
	struct tuiobackend *tuio = (struct tuiobackend *)container_of(base, struct tuiobackend, base);
	struct tuionode *node, *next;

	wl_list_remove(&base->link);

	wl_list_for_each_safe(node, next, &tuio->compz->tuio_list, link) {
		tuio_destroy_node(node);
	}

	free(tuio);
}
