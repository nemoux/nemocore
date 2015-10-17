#ifndef	__PROC_HELPER_H__
#define	__PROC_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int proc_get_process_name(pid_t pid, char *name, int size);
extern int proc_get_process_parent_id(pid_t pid, pid_t *ppid);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
