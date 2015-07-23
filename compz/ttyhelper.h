#ifndef	__TTY_HELPER_H__
#define	__TTY_HELPER_H__

#ifndef	KDSKBMUTE
#define	KDSKBMUTE	0x4b51
#endif

extern int tty_connect(int tty, int *kbmode);
extern void tty_restore(int fd, int kbmode);

#endif
