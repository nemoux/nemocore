#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <clipboard.h>
#include <datadevice.h>
#include <dataselection.h>
#include <dataoffer.h>
#include <compz.h>
#include <seat.h>
#include <keyboard.h>
#include <nemomisc.h>

static void clipboard_source_destroy(struct clipsource *source);

static int clipboard_dispatch_client_data(int fd, uint32_t mask, void *data)
{
	struct clipclient *client = (struct clipclient *)data;
	char *p;
	size_t size;
	int len;

	size = client->source->contents.size;
	p = client->source->contents.data;
	len = write(fd, p + client->offset, size - client->offset);
	if (len > 0)
		client->offset += len;

	if (client->offset == size || len <= 0) {
		close(fd);
		wl_event_source_remove(client->event_source);
		clipboard_source_destroy(client->source);
		free(client);
	}

	return 1;
}

static void clipboard_send_client(struct clipsource *source, int fd)
{
}

static void clipboard_source_accept(struct nemodatasource *base, uint32_t time, const char *mime_type)
{
}

static void clipboard_source_send(struct nemodatasource *base, const char *mime_type, int32_t fd)
{
	struct clipsource *source = (struct clipsource *)container_of(base, struct clipsource, base);
	char **s;

	s = source->base.mime_types.data;
	if (!strcmp(mime_type, s[0])) {
		struct nemoseat *seat = source->clipboard->seat;
		struct clipclient *client;

		client = (struct clipclient *)malloc(sizeof(struct clipclient));
		if (client == NULL)
			return;
		memset(client, 0, sizeof(struct clipclient));

		client->offset = 0;
		client->source = source;
		source->refcount++;
		client->event_source = wl_event_loop_add_fd(seat->compz->loop,
				fd, WL_EVENT_WRITABLE,
				clipboard_dispatch_client_data, client);
	} else {
		close(fd);
	}
}

static void clipboard_source_cancel(struct nemodatasource *base)
{
}

static int clipboard_dispatch_source_data(int fd, uint32_t mask, void *data)
{
	struct clipsource *source = (struct clipsource *)data;
	struct clipboard *clipboard = source->clipboard;
	char *p;
	int len, size;

	if (source->contents.alloc - source->contents.size < 1024) {
		wl_array_add(&source->contents, 1024);
		source->contents.size -= 1024;
	}

	p = source->contents.data + source->contents.size;
	size = source->contents.alloc - source->contents.size;
	len = read(fd, p, size);
	if (len == 0) {
		wl_event_source_remove(source->event_source);
		close(fd);
		source->event_source = NULL;
	} else if (len < 0) {
		clipboard_source_destroy(source);
		clipboard->source = NULL;
	} else {
		source->contents.size += len;
	}

	return 1;
}

static struct clipsource *clipboard_source_create(struct clipboard *clipboard, const char *mime_type, uint32_t serial, int fd)
{
	struct clipsource *source;
	char **s;

	source = (struct clipsource *)malloc(sizeof(struct clipsource));
	if (source == NULL)
		return NULL;
	memset(source, 0, sizeof(struct clipsource));

	wl_array_init(&source->contents);
	wl_array_init(&source->base.mime_types);

	source->base.resource = NULL;
	source->base.accept = clipboard_source_accept;
	source->base.send = clipboard_source_send;
	source->base.cancel = clipboard_source_cancel;

	wl_signal_init(&source->base.destroy_signal);

	source->refcount = 1;
	source->clipboard = clipboard;
	source->serial;

	s = wl_array_add(&source->base.mime_types, sizeof(char *));
	if (s == NULL)
		goto err1;
	*s = strdup(mime_type);
	if (*s == NULL)
		goto err2;

	source->event_source = wl_event_loop_add_fd(clipboard->seat->compz->loop,
			fd, WL_EVENT_READABLE,
			clipboard_dispatch_source_data, source);
	if (source->event_source == NULL)
		goto err2;

	return source;

err2:
	free(*s);

err1:
	wl_array_release(&source->contents);
	wl_array_release(&source->base.mime_types);

	free(source);

	return NULL;
}

static void clipboard_source_destroy(struct clipsource *source)
{
	char **s;

	if (--source->refcount > 0)
		return;

	if (source->event_source != NULL) {
		wl_event_source_remove(source->event_source);
		close(source->fd);
	}

	wl_signal_emit(&source->base.destroy_signal, &source->base);

	s = source->base.mime_types.data;
	free(*s);

	wl_array_release(&source->base.mime_types);
	wl_array_release(&source->contents);

	free(source);
}

static void clipboard_handle_selection(struct wl_listener *listener, void *data)
{
	struct clipboard *clipboard = (struct clipboard *)container_of(listener, struct clipboard, selection_listener);
	struct nemoseat *seat = (struct nemoseat *)data;
	struct nemodatasource *source = seat->selection.data_source;
	const char **mime_types;
	int p[2];

	if (source == NULL) {
		if (clipboard->source != NULL) {
			dataselection_set_selection(seat, &clipboard->source->base, clipboard->source->serial);
		}

		return;
	} else if (source->accept == clipboard_source_accept) {
		return;
	}

	if (clipboard->source != NULL)
		clipboard_source_destroy(clipboard->source);

	clipboard->source = NULL;

	mime_types = source->mime_types.data;

	if (pipe2(p, O_CLOEXEC) == -1)
		return;

	source->send(source, mime_types[0], p[1]);

	clipboard->source = clipboard_source_create(clipboard, mime_types[0], seat->selection.serial, p[0]);
	if (clipboard->source == NULL) {
		close(p[0]);
	}
}

static void clipboard_handle_destroy(struct wl_listener *listener, void *data)
{
	struct clipboard *clipboard = (struct clipboard *)container_of(listener, struct clipboard, destroy_listener);

	wl_list_remove(&clipboard->selection_listener.link);
	wl_list_remove(&clipboard->destroy_listener.link);

	free(clipboard);
}

struct clipboard *clipboard_create(struct nemoseat *seat)
{
	struct clipboard *clipboard;

	clipboard = (struct clipboard *)malloc(sizeof(struct clipboard));
	if (clipboard == NULL)
		return NULL;
	memset(clipboard, 0, sizeof(struct clipboard));

	clipboard->seat = seat;
	clipboard->selection_listener.notify = clipboard_handle_selection;
	clipboard->destroy_listener.notify = clipboard_handle_destroy;

	wl_signal_add(&seat->selection.signal, &clipboard->selection_listener);
	wl_signal_add(&seat->destroy_signal, &clipboard->destroy_listener);

	return clipboard;
}
