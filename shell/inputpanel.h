#ifndef	__NEMO_INPUTPANEL_H__
#define	__NEMO_INPUTPANEL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoshell;
struct nemolayer;

struct inputbin {
	struct wl_resource *resource;
	struct wl_signal destroy_signal;

	struct nemoshell *shell;

	struct wl_list link;

	struct nemocanvas *canvas;
	struct nemoview *view;
	struct nemolayer *layer;
	struct wl_listener canvas_destroy_listener;

	struct {
		float x, y;
		int32_t width, height;
	} screen;

	uint32_t panel;
};

extern int inputpanel_prepare(struct nemoshell *shell);
extern void inputpanel_finish(struct nemoshell *shell);

extern struct inputbin *inputpanel_get_bin(struct nemocanvas *canvas);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
