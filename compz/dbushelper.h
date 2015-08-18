#ifndef	__DBUS_HELPER_H__
#define	__DBUS_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <dbus/dbus.h>
#include <wayland-server.h>

extern int dbus_open_wayland(struct wl_event_loop *loop, DBusBusType bus, DBusConnection **conn, struct wl_event_source **src);
extern void dbus_close_wayland(DBusConnection *c, struct wl_event_source *source);

extern int dbus_add_match_vargs(DBusConnection *c, const char *format, ...);
extern int dbus_add_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path);
extern void dbus_remove_match_vargs(DBusConnection *c, const char *format, ...);
extern void dbus_remove_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
