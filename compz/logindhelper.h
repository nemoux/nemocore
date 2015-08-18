#ifndef	__LOGIND_HELPER_H__
#define	__LOGIND_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <compz.h>
#include <dbushelper.h>

struct logindcontext {
	struct nemocompz *compz;

	char *seat;
	char *sid;
	unsigned int vtnr;
	int vt;
	int kbmode;
	int sfd;
	struct wl_event_source *signal_source;

	DBusConnection *dbus;
	struct wl_event_source *dbus_source;

	char *spath;
	DBusPendingCall *pending_active;
};

extern struct logindcontext *logind_connect(struct nemocompz *compz, const char *seatid, int tty);
extern void logind_destroy(struct logindcontext *context);

extern void logind_restore(struct logindcontext *context);
extern int logind_activate_vt(struct logindcontext *context, int vt);

extern int logind_open(struct logindcontext *context, const char *path, int flags);
extern void logind_close(struct logindcontext *context, int fd);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
