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

struct nemobackend *tuiobackend_create(struct nemocompz *compz)
{
	struct tuiobackend *tuio;
	struct tuionode *node;
	struct itemone *one;
	int protocol, port, max;
	int i;

	tuio = (struct tuiobackend *)malloc(sizeof(struct tuiobackend));
	if (tuio == NULL)
		return NULL;
	memset(tuio, 0, sizeof(struct tuiobackend));

	tuio->base.destroy = tuiobackend_destroy;

	tuio->compz = compz;

	nemoitem_for_each(one, compz->configs) {
		if (nemoitem_one_has_path(one, "/nemoshell/tuio") != 0) {
			if (nemoitem_one_has_attr(one, "protocol", "osc") != 0)
				protocol = NEMOTUIO_OSC_PROTOCOL;
			else
				protocol = NEMOTUIO_XML_PROTOCOL;

			port = nemoitem_one_get_iattr(one, "port", 3333);
			max = nemoitem_one_get_iattr(one, "max", 16);

			node = tuio_create_node(compz, protocol, port, max);
			if (node == NULL)
				break;
		}
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
