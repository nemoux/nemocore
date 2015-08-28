#ifndef	__NEMO_PLUGIN_H__
#define	__NEMO_PLUGIN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemocompz;

extern int nemocompz_load_plugin(struct nemocompz *compz, const char *path);
extern void nemocompz_load_plugins(struct nemocompz *compz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
