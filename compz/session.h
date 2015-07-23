#ifndef __NEMO_SESSION_H__
#define	__NEMO_SESSION_H__

#include <compz.h>
#include <ttyhelper.h>

#ifdef NEMOUX_WITH_LOGIND
#include <logindhelper.h>
#endif

struct nemosession {
	struct nemocompz *compz;

#ifdef NEMOUX_WITH_LOGIND
	struct logindcontext *logind;
#endif

	int ttyfd, kbmode;
	int sfd;
	struct wl_event_source *signal_source;
};

extern struct nemosession *nemosession_create(struct nemocompz *compz);
extern void nemosession_destroy(struct nemosession *session);

extern int nemosession_connect(struct nemosession *session, const char *seatid, int tty);
extern int nemosession_activate_vt(struct nemosession *session, int vt);

extern int nemosession_open(struct nemosession *session, const char *path, int flags);
extern void nemosession_close(struct nemosession *session, int fd);

#endif
