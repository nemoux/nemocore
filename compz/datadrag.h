#ifndef	__NEMO_DATADRAG_H__
#define	__NEMO_DATADRAG_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <datadevice.h>
#include <pointer.h>
#include <touch.h>

struct nemodatasource;
struct nemocanvas;

struct nemodatadrag {
	union {
		struct nemopointer_grab pointer;
		struct touchpoint_grab touchpoint;
	} base;

	struct wl_client *client;
	struct nemodatasource *data_source;
	struct wl_listener data_source_listener;
	struct nemoview *focus;
	struct wl_resource *focus_resource;
	struct wl_listener focus_listener;
	struct nemoview *icon;
	struct wl_listener icon_destroy_listener;
	int32_t dx, dy;
};

extern int datadrag_start_pointer_grab(struct nemopointer *pointer, struct nemodatasource *source, struct nemocanvas *icon, struct wl_client *client);
extern int datadrag_start_touchpoint_grab(struct touchpoint *tp, struct nemodatasource *source, struct nemocanvas *icon, struct wl_client *client);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
