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
	lo_timetag timetag;
	lo_bundle bundle;
	lo_message alive;
	lo_message sets[NEMOCOMPZ_VIRTUIO_TOUCH_MAX];
	lo_message fseq;
	int nsets = 0;
	int i;

	lo_timetag_now(&timetag);

	bundle = lo_bundle_new(timetag);
	alive = lo_message_new();
	fseq = lo_message_new();

	lo_message_add_string(alive, "alive");
	lo_message_add_string(fseq, "fseq");

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		wl_list_for_each(tp, &touch->touchpoint_list, link) {
			if (tp->x < vtuio->x ||
					tp->y < vtuio->y ||
					tp->x >= vtuio->x + vtuio->width ||
					tp->y >= vtuio->y + vtuio->height)
				continue;

			lo_message_add_int32(alive, tp->gid);

			sets[nsets] = lo_message_new();

			lo_message_add_string(sets[nsets], "set");
			lo_message_add_int32(sets[nsets], tp->gid);
			lo_message_add_float(sets[nsets], (tp->x - vtuio->x) / vtuio->width);
			lo_message_add_float(sets[nsets], (tp->y - vtuio->y) / vtuio->height);
			lo_message_add_float(sets[nsets], 0.0f);
			lo_message_add_float(sets[nsets], 0.0f);
			lo_message_add_float(sets[nsets], 0.0f);

			nsets++;
		}
	}

	lo_message_add_int32(fseq, vtuio->fseq++);

	lo_bundle_add_message(bundle, "/tuio/2Dcur", alive);
	for (i = 0; i < nsets; i++)
		lo_bundle_add_message(bundle, "/tuio/2Dcur", sets[i]);
	lo_bundle_add_message(bundle, "/tuio/2Dcur", fseq);

	lo_send_bundle(vtuio->addr, bundle);

	lo_message_free(alive);
	for (i = 0; i < nsets; i++)
		lo_message_free(sets[i]);
	lo_message_free(fseq);
	lo_bundle_free(bundle);

	if (nsets > 0)
		wl_event_source_timer_update(vtuio->timer, 1000 / vtuio->fps);
	else
		vtuio->waiting = 1;

	return 1;
}

struct virtuio *virtuio_create(struct nemocompz *compz, int port, int fps, int x, int y, int width, int height)
{
	struct virtuio *vtuio;
	char str[32];

	vtuio = (struct virtuio *)malloc(sizeof(struct virtuio));
	if (vtuio == NULL)
		return NULL;
	memset(vtuio, 0, sizeof(struct virtuio));

	vtuio->compz = compz;
	vtuio->fps = fps;

	vtuio->x = x;
	vtuio->y = y;
	vtuio->width = width > 0 ? width : nemocompz_get_scene_width(compz);
	vtuio->height = height > 0 ? height : nemocompz_get_scene_height(compz);

	vtuio->waiting = 1;

	snprintf(str, sizeof(str), "%d", port);
	vtuio->addr = lo_address_new(NULL, str);

	vtuio->timer = wl_event_loop_add_timer(compz->loop, virtuio_dispatch_timer, vtuio);
	if (vtuio->timer == NULL)
		goto err1;

	wl_list_insert(compz->virtuio_list.prev, &vtuio->link);

	return vtuio;

err1:
	free(vtuio);

	return NULL;
}

void virtuio_destroy(struct virtuio *vtuio)
{
	wl_event_source_remove(vtuio->timer);

	free(vtuio);
}

void virtuio_dispatch_events(struct nemocompz *compz)
{
	struct virtuio *vtuio;

	wl_list_for_each(vtuio, &compz->virtuio_list, link) {
		if (vtuio->waiting != 0) {
			wl_event_source_timer_update(vtuio->timer, 1000 / vtuio->fps);

			vtuio->waiting = 0;
		}
	}
}
