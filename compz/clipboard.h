#ifndef	__NEMO_CLIPBOARD_H__
#define	__NEMO_CLIPBOARD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <datadevice.h>

struct nemoseat;

struct clipsource {
	struct nemodatasource base;
	struct wl_array contents;
	struct clipboard *clipboard;
	struct wl_event_source *event_source;
	uint32_t serial;
	int refcount;
	int fd;
};

struct clipclient {
	struct wl_event_source *event_source;
	size_t offset;
	struct clipsource *source;
};

struct clipboard {
	struct nemoseat *seat;
	struct wl_listener selection_listener;
	struct wl_listener destroy_listener;
	struct clipsource *source;
};

extern struct clipboard *clipboard_create(struct nemoseat *seat);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
