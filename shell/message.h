#ifndef __NEMO_SHELL_MESSAGE_H__
#define __NEMO_SHELL_MESSAGE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoitem.h>

extern int nemoshell_dispatch_message(void *data, const char *cmd, const char *path, struct itemone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
