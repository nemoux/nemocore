#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <renderer.h>
#include <compz.h>

void rendernode_prepare(struct rendernode *node)
{
	struct nemocompz *compz = node->compz;

	wl_list_init(&node->link);

	node->id = ffs(~compz->render_idpool) - 1;
	compz->render_idpool |= 1 << node->id;
}

void rendernode_finish(struct rendernode *node)
{
	node->compz->render_idpool &= ~(1 << node->id);
}
