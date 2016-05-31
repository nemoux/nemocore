#ifndef	__NEMO_MONITOR_H__
#define	__NEMO_MONITOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

typedef int (*nemomonitor_callback_t)(void *data);

struct nemomonitor {
	struct wl_event_source *source;

	nemomonitor_callback_t callback;
	void *data;
};

extern struct nemomonitor *nemomonitor_create(struct nemocompz *compz, int fd, nemomonitor_callback_t callback, void *data);
extern void nemomonitor_destroy(struct nemomonitor *monitor);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
