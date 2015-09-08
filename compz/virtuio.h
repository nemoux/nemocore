#ifndef __NEMO_VIRTUIO_H__
#define __NEMO_VIRTUIO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

struct nemocompz;

struct virtuio {
	struct nemocompz *compz;

	int fd;

	int fps;
	struct wl_event_source *timer;

	struct wl_list link;
};

extern struct virtuio *virtuio_create(struct nemocompz *compz, int port, int fps);
extern void virtuio_destroy(struct virtuio *vtuio);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
