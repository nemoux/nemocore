#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wayland-server.h>

#include <virtuio.h>
#include <compz.h>
#include <seat.h>
#include <touch.h>
#include <nemoitem.h>
#include <nemomisc.h>

static int virtuio_dispatch_timer(void *data)
{
	struct virtuio *vtuio = (struct virtuio *)data;
	struct nemocompz *compz = vtuio->compz;
	struct nemoseat *seat = compz->seat;
	struct nemotouch *touch;
	struct touchpoint *tp;

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
		}
	}

	wl_event_source_timer_update(vtuio->timer, 1000 / vtuio->fps);

	return 1;
}

struct virtuio *virtuio_create(struct nemocompz *compz, int port, int fps)
{
	struct virtuio *vtuio;
	struct sockaddr_in addr;
	socklen_t len;
	int r;

	vtuio = (struct virtuio *)malloc(sizeof(struct virtuio));
	if (vtuio == NULL)
		return NULL;
	memset(vtuio, 0, sizeof(struct virtuio));

	vtuio->compz = compz;

	vtuio->fps = fps;

	vtuio->fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (vtuio->fd < 0)
		goto err1;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(port);

	r = bind(vtuio->fd, (struct sockaddr *)&addr, sizeof(addr));
	if (r < 0)
		goto err2;

	vtuio->timer = wl_event_loop_add_timer(compz->loop, virtuio_dispatch_timer, vtuio);
	if (vtuio->timer == NULL)
		goto err2;
	wl_event_source_timer_update(vtuio->timer, 1000 / vtuio->fps);

	wl_list_insert(compz->virtuio_list.prev, &vtuio->link);

	return vtuio;

err2:
	close(vtuio->fd);

err1:
	free(vtuio);

	return NULL;
}

void virtuio_destroy(struct virtuio *vtuio)
{
	wl_event_source_remove(vtuio->timer);

	close(vtuio->fd);

	free(vtuio);
}
