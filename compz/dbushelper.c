#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

#include <dbus/dbus.h>
#include <wayland-server.h>

#include <dbushelper.h>

static int dbus_dispatch_watch_wayland(int fd, uint32_t mask, void *data)
{
	DBusWatch *watch = (DBusWatch *)data;
	uint32_t flags = 0;

	if (dbus_watch_get_enabled(watch)) {
		if (mask & WL_EVENT_READABLE)
			flags |= DBUS_WATCH_READABLE;
		if (mask & WL_EVENT_WRITABLE)
			flags |= DBUS_WATCH_WRITABLE;
		if (mask & WL_EVENT_HANGUP)
			flags |= DBUS_WATCH_HANGUP;
		if (mask & WL_EVENT_ERROR)
			flags |= DBUS_WATCH_ERROR;

		dbus_watch_handle(watch, flags);
	}

	return 1;
}

static dbus_bool_t dbus_add_watch_wayland(DBusWatch *watch, void *data)
{
	struct wl_event_loop *loop = (struct wl_event_loop *)data;
	struct wl_event_source *source;
	uint32_t mask = 0, flags = 0;
	int fd;

	if (dbus_watch_get_enabled(watch)) {
		flags = dbus_watch_get_flags(watch);
		if (flags & DBUS_WATCH_READABLE)
			mask |= WL_EVENT_READABLE;
		if (flags & DBUS_WATCH_WRITABLE)
			mask |= WL_EVENT_WRITABLE;
	}

	fd = dbus_watch_get_unix_fd(watch);
	source = wl_event_loop_add_fd(loop, fd, mask, dbus_dispatch_watch_wayland, watch);
	if (source == NULL)
		return FALSE;

	dbus_watch_set_data(watch, source, NULL);

	return TRUE;
}

static void dbus_remove_watch_wayland(DBusWatch *watch, void *data)
{
	struct wl_event_source *source;

	source = dbus_watch_get_data(watch);
	if (source == NULL)
		return;

	wl_event_source_remove(source);
}

static void dbus_toggle_watch_wayland(DBusWatch *watch, void *data)
{
	struct wl_event_source *source;
	uint32_t mask = 0, flags = 0;

	source = dbus_watch_get_data(watch);
	if (source == NULL)
		return;

	if (dbus_watch_get_enabled(watch)) {
		flags = dbus_watch_get_flags(watch);
		if (flags & DBUS_WATCH_READABLE)
			mask |= WL_EVENT_READABLE;
		if (flags & DBUS_WATCH_WRITABLE)
			mask |= WL_EVENT_WRITABLE;
	}

	wl_event_source_fd_update(source, mask);
}

static int dbus_dispatch_timeout_wayland(void *data)
{
	DBusTimeout *timeout = (DBusTimeout *)data;

	if (dbus_timeout_get_enabled(timeout))
		dbus_timeout_handle(timeout);

	return 1;
}

static int dbus_adjust_timeout_wayland(DBusTimeout *timeout, struct wl_event_source *source)
{
	int64_t t = 0;

	if (dbus_timeout_get_enabled(timeout))
		t = dbus_timeout_get_interval(timeout);

	return wl_event_source_timer_update(source, t);
}

static dbus_bool_t dbus_add_timeout_wayland(DBusTimeout *timeout, void *data)
{
	struct wl_event_loop *loop = (struct wl_event_loop *)data;
	struct wl_event_source *source;
	int r;

	source = wl_event_loop_add_timer(loop, dbus_dispatch_timeout_wayland, timeout);
	if (source == NULL)
		return FALSE;

	r = dbus_adjust_timeout_wayland(timeout, source);
	if (r < 0) {
		wl_event_source_remove(source);
		return FALSE;
	}

	dbus_timeout_set_data(timeout, source, NULL);

	return TRUE;
}

static void dbus_remove_timeout_wayland(DBusTimeout *timeout, void *data)
{
	struct wl_event_source *source;

	source = dbus_timeout_get_data(timeout);
	if (source == NULL)
		return;

	wl_event_source_remove(source);
}

static void dbus_toggle_timeout_wayland(DBusTimeout *timeout, void *data)
{
	struct wl_event_source *source;

	source = dbus_timeout_get_data(timeout);
	if (source == NULL)
		return;

	dbus_adjust_timeout_wayland(timeout, source);
}

static int dbus_dispatch_wayland(int fd, uint32_t mask, void *data)
{
	DBusConnection *c = (DBusConnection *)data;
	int r;

	do {
		r = dbus_connection_dispatch(c);
		if (r == DBUS_DISPATCH_COMPLETE)
			r = 0;
		else if (r == DBUS_DISPATCH_DATA_REMAINS)
			r = -EAGAIN;
		else if (r == DBUS_DISPATCH_NEED_MEMORY)
			r = -ENOMEM;
		else
			r = -EIO;
	} while (r == -EAGAIN);

	return 1;
}

static struct wl_event_source *dbus_bind_wayland(struct wl_event_loop *loop, DBusConnection *c)
{
	struct wl_event_source *source;
	int r, fd;

	fd = eventfd(0, EFD_CLOEXEC);
	if (fd < 0)
		return NULL;

	source = wl_event_loop_add_fd(loop, fd, 0, dbus_dispatch_wayland, c);
	if (source == NULL) {
		close(fd);
		return NULL;
	}

	close(fd);

	wl_event_source_check(source);

	r = dbus_connection_set_watch_functions(c,
			dbus_add_watch_wayland,
			dbus_remove_watch_wayland,
			dbus_toggle_watch_wayland,
			loop,
			NULL);
	if (!r)
		goto err1;

	r = dbus_connection_set_timeout_functions(c,
			dbus_add_timeout_wayland,
			dbus_remove_timeout_wayland,
			dbus_toggle_timeout_wayland,
			loop,
			NULL);
	if (!r)
		goto err1;

	dbus_connection_ref(c);

	return source;

err1:
	dbus_connection_set_timeout_functions(c, NULL, NULL, NULL, NULL, NULL);
	dbus_connection_set_watch_functions(c, NULL, NULL, NULL, NULL, NULL);

	wl_event_source_remove(source);

	return NULL;
}

static void dbus_unbind_wayland(DBusConnection *c, struct wl_event_source *source)
{
	dbus_connection_set_timeout_functions(c, NULL, NULL, NULL, NULL, NULL);
	dbus_connection_set_watch_functions(c, NULL, NULL, NULL, NULL, NULL);

	dbus_connection_unref(c);

	wl_event_source_remove(source);
}

int dbus_open_wayland(struct wl_event_loop *loop, DBusBusType bus, DBusConnection **conn, struct wl_event_source **src)
{
	DBusConnection *c;
	struct wl_event_source *source;
	int r = 0;

	dbus_connection_set_change_sigpipe(FALSE);

	c = dbus_bus_get_private(bus, NULL);
	if (c == NULL)
		return -EIO;

	dbus_connection_set_exit_on_disconnect(c, FALSE);

	source = dbus_bind_wayland(loop, c);
	if (source == NULL)
		goto err1;

	*conn = c;
	*src = source;

	return r;

err1:
	dbus_connection_close(c);
	dbus_connection_unref(c);

	return -EINVAL;
}

void dbus_close_wayland(DBusConnection *c, struct wl_event_source *source)
{
	dbus_unbind_wayland(c, source);

	dbus_connection_close(c);
	dbus_connection_unref(c);
}

int dbus_add_match_vargs(DBusConnection *c, const char *format, ...)
{
	DBusError err;
	va_list list;
	char *str;
	int r;

	va_start(list, format);
	r = vasprintf(&str, format, list);
	va_end(list);

	if (r < 0)
		return -ENOMEM;

	dbus_error_init(&err);
	dbus_bus_add_match(c, str, &err);

	free(str);

	if (dbus_error_is_set(&err)) {
		dbus_error_free(&err);
		return -EIO;
	}

	return 0;
}

int dbus_add_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path)
{
	return dbus_add_match_vargs(c,
			"type='signal',"
			"sender='%s',"
			"interface='%s',"
			"member='%s',"
			"path='%s'",
			sender, iface, member, path);
}

void dbus_remove_match_vargs(DBusConnection *c, const char *format, ...)
{
	va_list list;
	char *str;
	int r;

	va_start(list, format);
	r = vasprintf(&str, format, list);
	va_end(list);

	if (r < 0)
		return;

	dbus_bus_remove_match(c, str, NULL);

	free(str);
}

void dbus_remove_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path)
{
	return dbus_remove_match_vargs(c,
			"type='signal',"
			"sender='%s',"
			"interface='%s',"
			"member='%s',"
			"path='%s'",
			sender, iface, member, path);
}
