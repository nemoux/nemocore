#ifndef	__SYS_HELPER_H__
#define	__SYS_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int sys_get_process_name(pid_t pid, char *name, int size);
extern int sys_get_process_parent_id(pid_t pid, pid_t *ppid);

extern float sys_get_cpu_usage(void);
extern float sys_get_memory_usage(void);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
