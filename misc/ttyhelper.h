#ifndef	__TTY_HELPER_H__
#define	__TTY_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#ifndef	KDSKBMUTE
#define	KDSKBMUTE	0x4b51
#endif

extern int tty_connect(int tty, int *kbmode);
extern void tty_restore(int fd, int kbmode);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
