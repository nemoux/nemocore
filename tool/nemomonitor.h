#ifndef __NEMOTOOL_MONITOR_H__
#define __NEMOTOOL_MONITOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemotool;

typedef int (*nemomonitor_callback_t)(void *data);

struct nemomonitor {
	struct nemotool *tool;
	int fd;

	nemomonitor_callback_t callback;
	void *data;
};

extern struct nemomonitor *nemomonitor_create(struct nemotool *tool, int fd, nemomonitor_callback_t callback, void *data);
extern void nemomonitor_destroy(struct nemomonitor *monitor);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
