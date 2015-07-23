#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
#include <systemd/sd-login.h>
#include <wayland-server.h>

#include <logindhelper.h>
#include <dbushelper.h>
#include <nemolog.h>

#ifndef DRM_MAJOR
#define	DRM_MAJOR	226
#endif

#ifndef	KDSKBMUTE
#define	KDSKBMUTE	0x4b51
#endif

static int logind_get_session_vt(const char *sid, unsigned int *vtnr)
{
#ifdef HAVE_SYSTEMD_LOGIN_209
	return sd_session_get_vt(sid, vtnr);
#else
	char *tty;
	int r;

	r = sd_session_get_tty(sid, &tty);
	if (r < 0)
		return r;

	r = sscanf(tty, "/dev/tty%u", vtnr);

	free(tty);

	if (r != 1)
		return -EINVAL;

	return 0;
#endif
}

int logind_take_device(struct logindcontext *context, uint32_t major, uint32_t minor, int *paused)
{
	DBusMessage *m, *reply;
	dbus_bool_t p;
	int r, fd;

	m = dbus_message_new_method_call("org.freedesktop.login1",
			context->spath,
			"org.freedesktop.login1.Session",
			"TakeDevice");
	if (m == NULL)
		return -ENOMEM;

	r = dbus_message_append_args(m,
			DBUS_TYPE_UINT32, &major,
			DBUS_TYPE_UINT32, &minor,
			DBUS_TYPE_INVALID);
	if (r == 0)
		goto err1;

	reply = dbus_connection_send_with_reply_and_block(context->dbus, m, -1, NULL);
	if (reply == NULL)
		goto err1;

	r = dbus_message_get_args(reply, NULL,
			DBUS_TYPE_UNIX_FD, &fd,
			DBUS_TYPE_BOOLEAN, &p,
			DBUS_TYPE_INVALID);
	if (r == 0)
		goto err2;

	if (paused != NULL)
		*paused = p;

	return fd;

err2:
	dbus_message_unref(reply);

err1:
	dbus_message_unref(m);

	return -1;
}

static void logind_release_device(struct logindcontext *context, uint32_t major, uint32_t minor)
{
	DBusMessage *m;
	int r;

	m = dbus_message_new_method_call("org.freedesktop.login1",
			context->spath,
			"org.freedesktop.login1.Session",
			"ReleaseDevice");
	if (m != NULL) {
		r = dbus_message_append_args(m,
				DBUS_TYPE_UINT32, &major,
				DBUS_TYPE_UINT32, &minor,
				DBUS_TYPE_INVALID);
		if (r != 0)
			dbus_connection_send(context->dbus, m, NULL);

		dbus_message_unref(m);
	}
}

static DBusHandlerResult logind_filter_dbus(DBusConnection *c, DBusMessage *m, void *data)
{
	struct logindcontext *context = (struct logindcontext *)data;

	if (dbus_message_is_signal(m, DBUS_INTERFACE_LOCAL, "Disconnected")) {
	} else if (dbus_message_is_signal(m, "org.freedesktop.login1.Manager", "SessionRemoved")) {
	} else if (dbus_message_is_signal(m, "org.freedesktop.DBus.Properties", "PropertiesChanged")) {
	} else if (dbus_message_is_signal(m, "org.freedesktop.login1.Session", "PauseDevice")) {
	} else if (dbus_message_is_signal(m, "org.freedesktop.login1.Session", "ResumeDevice")) {
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int logind_setup_dbus(struct logindcontext *context)
{
	int r;

	r = asprintf(&context->spath, "/org/freedesktop/login1/session/%s", context->sid);
	if (r < 0)
		return -ENOMEM;

	r = dbus_connection_add_filter(context->dbus, logind_filter_dbus, context, NULL);
	if (r == 0) {
		nemolog_error("LOGIND", "failed to add dbus filter\n");
		r = -ENOMEM;
		goto err1;
	}

	r = dbus_add_match_signal(context->dbus,
			"org.freedesktop.login1",
			"org.freedesktop.login1.Manager",
			"SessionRemoved",
			"/org/freedesktop/login1");
	if (r < 0) {
		nemolog_error("LOGIND", "failed to add dbus match\n");
		goto err1;
	}

	r = dbus_add_match_signal(context->dbus,
			"org.freedesktop.login1",
			"org.freedesktop.login1.Manager",
			"PauseDevice",
			context->spath);
	if (r < 0) {
		nemolog_error("LOGIND", "failed to add dbus match\n");
		goto err1;
	}

	r = dbus_add_match_signal(context->dbus,
			"org.freedesktop.login1",
			"org.freedesktop.login1.Manager",
			"ResumeDevice",
			context->spath);
	if (r < 0) {
		nemolog_error("LOGIND", "failed to add dbus match\n");
		goto err1;
	}

	r = dbus_add_match_signal(context->dbus,
			"org.freedesktop.login1",
			"org.freedesktop.login1.Manager",
			"PropertiesChanged",
			context->spath);
	if (r < 0) {
		nemolog_error("LOGIND", "failed to add dbus match\n");
		goto err1;
	}

	return 0;

err1:
	free(context->spath);

	return r;
}

static void logind_destroy_dbus(struct logindcontext *context)
{
	free(context->spath);
}

static int logind_take_control(struct logindcontext *context)
{
	DBusMessage *m, *reply;
	DBusError err;
	dbus_bool_t force;
	int r;

	dbus_error_init(&err);

	m = dbus_message_new_method_call("org.freedesktop.login1",
			context->spath,
			"org.freedesktop.login1.Session",
			"TakeControl");
	if (m == NULL)
		return -ENOMEM;

	force = false;
	r = dbus_message_append_args(m,
			DBUS_TYPE_BOOLEAN, &force,
			DBUS_TYPE_INVALID);
	if (r == 0) {
		r = -ENOMEM;
		goto err1;
	}

	reply = dbus_connection_send_with_reply_and_block(context->dbus, m, -1, &err);
	if (reply == NULL) {
		if (dbus_error_has_name(&err, DBUS_ERROR_UNKNOWN_METHOD)) {
			nemolog_error("LOGIND", "old systemd version detected\n");
		} else {
			nemolog_error("LOGIND", "failed to take control over session %s\n", context->sid);
		}

		dbus_error_free(&err);
		r = -EIO;
		goto err1;
	}

	dbus_message_unref(reply);
	dbus_message_unref(m);

	return 0;

err1:
	dbus_message_unref(m);

	return r;
}

static void logind_release_control(struct logindcontext *context)
{
	DBusMessage *m;

	m = dbus_message_new_method_call("org.freedesktop.login1",
			context->spath,
			"org.freedesktop.login1.Session",
			"ReleaseControl");
	if (m != NULL) {
		dbus_connection_send(context->dbus, m, NULL);
		dbus_message_unref(m);
	}
}

static int logind_dispatch_signal(int fd, uint32_t mask, void *data)
{
	struct logindcontext *context = (struct logindcontext *)data;
	struct nemocompz *compz = context->compz;
	struct signalfd_siginfo sig;

	if (read(fd, &sig, sizeof(struct signalfd_siginfo)) != sizeof(struct signalfd_siginfo)) {
		nemolog_error("LOGIND", "failed to read signalfd\n");
		return 0;
	}

	switch (sig.ssi_signo) {
		case SIGUSR1:
			compz->session_active = 0;
			wl_signal_emit(&compz->session_signal, compz);
			ioctl(context->vt, VT_RELDISP, 1);
			break;

		case SIGUSR2:
			ioctl(context->vt, VT_RELDISP, VT_ACKACQ);
			compz->session_active = 1;
			wl_signal_emit(&compz->session_signal, compz);
			break;
	}

	return 1;
}

static int logind_setup_vt(struct logindcontext *context)
{
	struct stat st;
	struct vt_mode mode = { 0 };
	sigset_t mask;
	char buf[64];
	int r;

	snprintf(buf, sizeof(buf), "/dev/tty%d", context->vtnr);

	context->vt = open(buf, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (context->vt < 0) {
		nemolog_error("LOGIND", "failed to open VT '%s'\n", buf);
		return -errno;
	}

	if (fstat(context->vt, &st) == -1 ||
			major(st.st_dev) != TTY_MAJOR ||
			minor(st.st_rdev) <= 0 || minor(st.st_rdev) >= 64) {
		nemolog_error("LOGIND", "VT '%s' is not virtual terminal\n", buf);
		r = -EINVAL;
		goto err1;
	}

	if (ioctl(context->vt, KDGKBMODE, &context->kbmode) < 0) {
		nemolog_error("LOGIND", "failed to keyboard mode on %s\n", buf);
		context->kbmode = K_UNICODE;
	} else if (context->kbmode == K_OFF) {
		context->kbmode = K_UNICODE;
	}

	if (ioctl(context->vt, KDSKBMUTE, 1) < 0 && ioctl(context->vt, KDSKBMODE, K_OFF) < 0) {
		nemolog_error("LOGIND", "failed to set K_OFF keyboard mode on %s\n", buf);
		r = -errno;
		goto err1;
	}

	if (ioctl(context->vt, KDSETMODE, KD_GRAPHICS) < 0) {
		nemolog_error("LOGIND", "failed to set KD_GRAPHICS mode on %s\n", buf);
		r = -errno;
		goto err2;
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	context->sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (context->sfd < 0) {
		nemolog_error("LOGIND", "failed to create signalfd\n");
		r = -errno;
		goto err3;
	}

	context->signal_source = wl_event_loop_add_fd(context->compz->loop,
			context->sfd,
			WL_EVENT_READABLE,
			logind_dispatch_signal,
			context);
	if (context->signal_source == NULL) {
		nemolog_error("LOGIND", "failed to create signalfd source\n");
		r = -errno;
		goto err4;
	}

	mode.mode = VT_PROCESS;
	mode.relsig = SIGUSR1;
	mode.acqsig = SIGUSR2;

	if (ioctl(context->vt, VT_SETMODE, &mode) < 0) {
		nemolog_error("LOGIND", "failed to take over VT\n");
		r = -errno;
		goto err5;
	}

	nemolog_error("LOGIND", "using VT %s\n", buf);

	return 0;

err5:
	wl_event_source_remove(context->signal_source);

err4:
	close(context->sfd);

err3:
	ioctl(context->vt, KDSETMODE, KD_TEXT);

err2:
	ioctl(context->vt, KDSKBMUTE, 0);
	ioctl(context->vt, KDSKBMODE, context->kbmode);

err1:
	close(context->vt);

	return r;
}

static void logind_destroy_vt(struct logindcontext *context)
{
	logind_restore(context);

	wl_event_source_remove(context->signal_source);
	close(context->sfd);
	close(context->vt);
}

struct logindcontext *logind_connect(struct nemocompz *compz, const char *seatid, int tty)
{
	struct logindcontext *context;
	char *t;
	int r;

	context = (struct logindcontext *)malloc(sizeof(struct logindcontext));
	if (context == NULL)
		return NULL;
	memset(context, 0, sizeof(struct logindcontext));

	context->compz = compz;

	context->seat = strdup(seatid);
	if (context->seat == NULL)
		goto err1;

	r = sd_pid_get_session(getpid(), &context->sid);
	if (r < 0) {
		nemolog_error("LOGIND", "not running in a systemd session\n");
		goto err2;
	}

	t = NULL;
	r = sd_session_get_seat(context->sid, &t);
	if (r < 0) {
		nemolog_error("LOGIND", "failed to get session seat\n");
		free(t);
		goto err3;
	} else if (strcmp(seatid, t) != 0) {
		nemolog_error("LOGIND", "owner's seat '%s' differs from session seat '%s'\n", seatid, t);
		free(t);
		goto err3;
	}
	free(t);

	r = logind_get_session_vt(context->sid, &context->vtnr);
	if (r < 0) {
		nemolog_error("LOGIND", "session is not running on a VT\n");
		goto err3;
	} else if (tty > 0 && context->vtnr != tty) {
		nemolog_error("LOGIND", "owner's tty '%d' differs from session VT '%d'\n", tty, context->vtnr);
		goto err3;
	}

	r = dbus_open_wayland(compz->loop, DBUS_BUS_SYSTEM, &context->dbus, &context->dbus_source);
	if (r < 0) {
		nemolog_error("LOGIND", "failed to connect to system dbus\n");
		goto err3;
	}

	r = logind_setup_dbus(context);
	if (r < 0)
		goto err4;

	r = logind_take_control(context);
	if (r < 0)
		goto err5;

	r = logind_setup_vt(context);
	if (r < 0)
		goto err6;

	return context;

err6:
	logind_release_control(context);

err5:
	logind_destroy_dbus(context);

err4:
	dbus_close_wayland(context->dbus, context->dbus_source);

err3:
	free(context->sid);

err2:
	free(context->seat);

err1:
	free(context);

	return NULL;
}

void logind_destroy(struct logindcontext *context)
{
	if (context->pending_active != NULL) {
		dbus_pending_call_cancel(context->pending_active);
		dbus_pending_call_unref(context->pending_active);
	}

	if (context->vt >= 0)
		logind_destroy_vt(context);
	logind_release_control(context);
	logind_destroy_dbus(context);

	dbus_close_wayland(context->dbus, context->dbus_source);

	free(context->sid);
	free(context->seat);
	free(context);
}

void logind_restore(struct logindcontext *context)
{
	struct vt_mode mode = { 0 };

	ioctl(context->vt, KDSETMODE, KD_TEXT);
	ioctl(context->vt, KDSKBMUTE, 0);
	ioctl(context->vt, KDSKBMODE, context->kbmode);

	mode.mode = VT_AUTO;

	ioctl(context->vt, VT_SETMODE, &mode);
}

int logind_activate_vt(struct logindcontext *context, int vt)
{
	return ioctl(context->vt, VT_ACTIVATE, vt);
}

int logind_open(struct logindcontext *context, const char *path, int flags)
{
	struct stat st;
	int fd, fl;

	if (stat(path, &st) < 0)
		return -1;
	if (!S_ISCHR(st.st_mode))
		return -1;

	fd = logind_take_device(context, major(st.st_rdev), minor(st.st_rdev), NULL);
	if (fd < 0)
		return fd;

	fl = fcntl(fd, F_GETFL);
	if (fl < 0)
		goto err1;

	if (flags & O_NONBLOCK)
		fl |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, fl) < 0)
		goto err1;

	fl = fcntl(fd, F_GETFD);
	if (fl < 0)
		goto err1;

	if (!(flags & O_CLOEXEC))
		fl &= ~FD_CLOEXEC;

	if (fcntl(fd, F_SETFD, fl) < 0)
		goto err1;

	return fd;

err1:
	close(fd);

	logind_release_device(context, major(st.st_rdev), minor(st.st_rdev));

	return -1;
}

void logind_close(struct logindcontext *context, int fd)
{
	struct stat st;

	if (fstat(fd, &st) < 0)
		return;

	if (!S_ISCHR(st.st_mode))
		return;

	logind_release_device(context, major(st.st_rdev), minor(st.st_rdev));
}
