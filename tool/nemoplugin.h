#ifndef __NEMOTOOL_PLUGIN_H__
#define	__NEMOTOOL_PLUGIN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemotool;
struct nemochannel;
struct nemoqueue;

extern int nemotool_load_plugin(struct nemotool *tool, const char *path, const char *args, struct nemochannel *chan, struct nemoqueue *queue);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
