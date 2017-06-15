#ifndef	__NEMO_SEAT_H__
#define	__NEMO_SEAT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoview;

struct nemoseat {
	struct nemocompz *compz;

	struct wl_signal destroy_signal;

	struct wl_list resource_list;
	struct wl_list drag_resource_list;

	struct {
		struct wl_list resource_list;
		struct wl_list nemo_resource_list;

		struct wl_list device_list;

		struct wl_signal focus_signal;
	} keyboard;

	struct {
		struct wl_list device_list;

		struct wl_signal focus_signal;
	} keypad;

	struct {
		struct wl_list resource_list;
		struct wl_list nemo_resource_list;

		struct wl_list device_list;

		struct wl_signal focus_signal;
	} pointer;

	struct {
		struct wl_list resource_list;
		struct wl_list nemo_resource_list;

		struct wl_list device_list;

		struct wl_signal focus_signal;
	} touch;

	struct {
		uint32_t serial;
		struct nemodatasource *data_source;
		struct wl_listener data_source_listener;
		struct wl_signal signal;

		struct nemoview *focus;
	} selection;
};

extern struct nemoseat *nemoseat_create(struct nemocompz *compz);
extern void nemoseat_destroy(struct nemoseat *seat);

extern struct nemopointer *nemoseat_get_first_pointer(struct nemoseat *seat);
extern struct nemokeyboard *nemoseat_get_first_keyboard(struct nemoseat *seat);

extern struct nemopointer *nemoseat_get_pointer_by_focus_serial(struct nemoseat *seat, uint32_t serial);
extern struct nemopointer *nemoseat_get_pointer_by_grab_serial(struct nemoseat *seat, uint32_t serial);
extern struct nemopointer *nemoseat_get_pointer_by_id(struct nemoseat *seat, uint64_t id);
extern int nemoseat_get_pointer_by_view(struct nemoseat *seat, struct nemoview *view, struct nemopointer *ptrs[], int max);
extern int nemoseat_has_pointer_resource_by_view(struct nemoseat *seat, struct nemoview *view);

extern struct nemokeyboard *nemoseat_get_keyboard_by_focus_serial(struct nemoseat *seat, uint32_t serial);
extern struct nemokeyboard *nemoseat_get_keyboard_by_id(struct nemoseat *seat, uint64_t id);
extern int nemoseat_get_keyboard_by_view(struct nemoseat *seat, struct nemoview *view, struct nemokeyboard *kbds[], int max);

extern struct nemokeypad *nemoseat_get_keypad_by_id(struct nemoseat *seat, uint64_t id);

extern struct touchpoint *nemoseat_get_touchpoint_by_grab_serial(struct nemoseat *seat, uint32_t serial);
extern struct touchpoint *nemoseat_get_touchpoint_by_id(struct nemoseat *seat, uint64_t id);
extern struct touchpoint *nemoseat_get_touchpoint_by_id_nocheck(struct nemoseat *seat, uint64_t id);
extern int nemoseat_get_touchpoint_by_view(struct nemoseat *seat, struct nemoview *view, struct touchpoint *tps[], int max);
extern int nemoseat_put_touchpoint_by_view(struct nemoseat *seat, struct nemoview *view);

extern struct wl_resource *nemoseat_find_resource_for_view(struct wl_list *list, struct nemoview *view);

extern void nemoseat_set_keyboard_focus(struct nemoseat *seat, struct nemoview *view);
extern void nemoseat_set_pointer_focus(struct nemoseat *seat, struct nemoview *view);
extern void nemoseat_put_focus(struct nemoseat *seat, struct nemoview *view);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
