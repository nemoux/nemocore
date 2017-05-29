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

#include <ttyhelper.h>
#include <nemolog.h>

int tty_connect(int tty, int *kbmode)
{
	struct vt_mode mode = { 0 };
	struct stat st;
	char ttyname[32] = "<stdin>";
	int fd, kdmode;

	if (kbmode == NULL)
		return -1;

	if (tty == 0) {
		fd = dup(0);
		if (fd < 0) {
			nemolog_error("TTY", "failed to open stdin for tty\n");
			return -1;
		}
	} else {
		snprintf(ttyname, sizeof(ttyname), "/dev/tty%d", tty);

		fd = open(ttyname, O_RDWR | O_CLOEXEC | O_NONBLOCK);
		if (fd < 0) {
			nemolog_error("TTY", "failed to open tty %s\n", ttyname);
			return -1;
		}
	}

	if (fstat(fd, &st) == -1 ||
			major(st.st_rdev) != TTY_MAJOR || minor(st.st_rdev) == 0) {
		nemolog_error("TTY", "%s is not virtual terminal\n", ttyname);
		goto err1;
	}

	if (ioctl(fd, KDGETMODE, &kdmode)) {
		nemolog_error("TTY", "failed to get VT mode\n");
		goto err1;
	}

	if (kdmode != KD_TEXT) {
		nemolog_error("TTY", "%s is already in graphics mode\n", ttyname);
		goto err1;
	}

	ioctl(fd, VT_ACTIVATE, minor(st.st_rdev));
	ioctl(fd, VT_WAITACTIVE, minor(st.st_rdev));

	if (ioctl(fd, KDGKBMODE, kbmode)) {
		nemolog_error("TTY", "failed to get keyboard mode\n");
	}

	if (ioctl(fd, KDSKBMUTE, 1) && ioctl(fd, KDSKBMODE, K_OFF)) {
		nemolog_error("TTY", "failed to set K_OFF Keyboard mode\n");
		goto err1;
	}

	if (ioctl(fd, KDSETMODE, KD_GRAPHICS) < 0) {
		nemolog_error("TTY", "failed to set KD_GRAPHICS mode on tty\n");
		goto err1;
	}

	mode.mode = VT_PROCESS;
	mode.relsig = SIGUSR1;
	mode.acqsig = SIGUSR2;

	if (ioctl(fd, VT_SETMODE, &mode) < 0) {
		nemolog_error("TTY", "failed to take control of vt handling\n");
		goto err1;
	}

	return fd;

err1:
	close(fd);

	return -1;
}

void tty_restore(int fd, int kbmode)
{
	struct vt_mode mode = { 0 };

	if (ioctl(fd, KDSKBMUTE, 0) && ioctl(fd, KDSKBMODE, kbmode))
		nemolog_error("SEAT", "failed to restore keyboard mode\n");

	if (ioctl(fd, KDSETMODE, KD_TEXT))
		nemolog_error("SEAT", "failed to set KD_TEXT mode on tty\n");

	mode.mode = VT_AUTO;
	if (ioctl(fd, VT_SETMODE, &mode) < 0)
		nemolog_error("SEAT", "failed to reset vt handling\n");
}
