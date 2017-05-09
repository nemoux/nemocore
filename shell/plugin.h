#ifndef	__NEMO_PLUGIN_H__
#define	__NEMO_PLUGIN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoshell;

extern int nemoshell_load_plugin(struct nemoshell *shell, const char *path, const char *args);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
