#ifndef	__NEMO_XWAYLAND_CURSOR_H__
#define	__NEMO_XWAYLAND_CURSOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoxmanager;

enum cursor_type {
	NEMOX_CURSOR_TOP,
	NEMOX_CURSOR_BOTTOM,
	NEMOX_CURSOR_LEFT,
	NEMOX_CURSOR_RIGHT,
	NEMOX_CURSOR_TOP_LEFT,
	NEMOX_CURSOR_TOP_RIGHT,
	NEMOX_CURSOR_BOTTOM_LEFT,
	NEMOX_CURSOR_BOTTOM_RIGHT,
	NEMOX_CURSOR_LEFT_PTR,
};

extern int nemoxmanager_create_cursors(struct nemoxmanager *xmanager);
extern void nemoxmanager_destroy_cursors(struct nemoxmanager *xmanager);

extern void nemoxmanager_set_cursor(struct nemoxmanager *xmanager, xcb_window_t wid, int cursor);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
