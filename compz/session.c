#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/major.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <wayland-server.h>

#include <session.h>
#include <compz.h>
#include <ttyhelper.h>
#include <nemolog.h>

#ifdef NEMOUX_WITH_LOGIND
#include <logindhelper.h>
#endif

struct nemosession *nemosession_create(struct nemocompz *compz)
{
	struct nemosession *session;

	session = (struct nemosession *)malloc(sizeof(struct nemosession));
	if (session == NULL)
		return NULL;
	memset(session, 0, sizeof(struct nemosession));

	session->compz = compz;
#ifdef NEMOUX_WITH_LOGIND
	session->logind = NULL;
#endif
	session->ttyfd = -1;

	return session;
}

void nemosession_destroy(struct nemosession *session)
{
#ifdef NEMOUX_WITH_LOGIND
	if (session->logind != NULL) {
		logind_destroy(session->logind);
	}
#endif
	if (session->ttyfd >= 0) {
		tty_restore(session->ttyfd, session->kbmode);
	}
	if (session->signal_source != NULL) {
		wl_event_source_remove(session->signal_source);
	}
	if (session->sfd >= 0) {
		close(session->sfd);
	}
	if (session->ttyfd >= 0) {
		close(session->ttyfd);
	}

	free(session);
}

int nemosession_activate_vt(struct nemosession *session, int vt)
{
#ifdef NEMOUX_WITH_LOGIND
	if (session->logind != NULL)
		return logind_activate_vt(session->logind, vt);
#endif

	return ioctl(session->ttyfd, VT_ACTIVATE, vt);
}

int nemosession_open(struct nemosession *session, const char *path, int flags)
{
	struct stat st;
	int fd;

#ifdef NEMOUX_WITH_LOGIND
	if (session->logind != NULL)
		return logind_open(session->logind, path, flags);
#endif

	fd = open(path, flags | O_CLOEXEC);
	if (fd < 0)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

void nemosession_close(struct nemosession *session, int fd)
{
#ifdef NEMOUX_WITH_LOGIND
	if (session->logind != NULL)
		return logind_close(session->logind, fd);
#endif

	close(fd);
}

static int nemocompz_dispatch_session_signal(int fd, uint32_t mask, void *data)
{
	struct nemosession *session = (struct nemosession *)data;
	struct nemocompz *compz = session->compz;
	struct signalfd_siginfo sig;

	if (read(fd, &sig, sizeof(struct signalfd_siginfo)) != sizeof(struct signalfd_siginfo)) {
		nemolog_error("LOGIND", "failed to read signalfd\n");
		return 0;
	}

	switch (sig.ssi_signo) {
		case SIGUSR1:
			compz->session_active = 0;
			wl_signal_emit(&compz->session_signal, compz);
			ioctl(session->ttyfd, VT_RELDISP, 1);
			break;

		case SIGUSR2:
			ioctl(session->ttyfd, VT_RELDISP, VT_ACKACQ);
			compz->session_active = 1;
			wl_signal_emit(&compz->session_signal, compz);
			break;
	}

	return 1;
}

int nemocompz_connect_session(struct nemocompz *compz, const char *seatid, int tty)
{
	struct nemosession *session = compz->session;

	if (seatid == NULL)
		return -1;

#ifdef NEMOUX_WITH_LOGIND
	session->logind = logind_connect(compz, seatid, tty);
	if (session->logind == NULL) {
		nemolog_error("LOGIND", "failed to connect logind (%s, %d)\n", seatid, tty);
#endif
		if (geteuid() == 0) {
			sigset_t mask;

			session->ttyfd = tty_connect(tty, &session->kbmode);
			if (session->ttyfd < 0) {
				nemolog_error("LOGIND", "failed to open tty %d\n", tty);
				return -1;
			}

			sigemptyset(&mask);
			sigaddset(&mask, SIGUSR1);
			sigaddset(&mask, SIGUSR2);
			sigprocmask(SIG_BLOCK, &mask, NULL);

			session->sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
			if (session->sfd < 0) {
				nemolog_error("LOGIND", "failed to create signal fd for vt event handling\n");
				close(session->ttyfd);
				return -1;
			}

			session->signal_source = wl_event_loop_add_fd(compz->loop,
					session->sfd,
					WL_EVENT_READABLE,
					nemocompz_dispatch_session_signal,
					session);
			if (session->signal_source == NULL) {
				nemolog_error("LOGIND", "failed to create vt event source\n");
				close(session->ttyfd);
				close(session->sfd);
				session->ttyfd = -1;
				session->sfd = -1;
				return -1;
			}
		}
#ifdef NEMOUX_WITH_LOGIND
	}
#endif

	return 0;
}
