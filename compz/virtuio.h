#ifndef __NEMO_VIRTUIO_H__
#define __NEMO_VIRTUIO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define	NEMOCOMPZ_VIRTUIO_TOUCH_MAX			(128)

#include <stdint.h>

#include <lo/lo.h>

struct nemocompz;

struct virtuio {
	struct nemocompz *compz;

	float x, y, width, height;

	int waiting;

	lo_address addr;

	int fps;
	struct wl_event_source *timer;

	uint64_t fseq;

	struct wl_list link;
};

extern struct virtuio *virtuio_create(struct nemocompz *compz, int port, int fps, int x, int y, int width, int height);
extern void virtuio_destroy(struct virtuio *vtuio);

extern void virtuio_dispatch_events(struct nemocompz *compz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
