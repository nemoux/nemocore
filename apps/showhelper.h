#ifndef	__NEMOTOOL_SHOW_HELPER_H__
#define	__NEMOTOOL_SHOW_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotool.h>
#include <nemotimer.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemohangul.h>

#include <nemoshow.h>

typedef enum {
	NEMOSHOW_VIEW_PICK_ROTATE_TYPE = (1 << 0),
	NEMOSHOW_VIEW_PICK_SCALE_TYPE = (1 << 1),
	NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE = (1 << 2),
	NEMOSHOW_VIEW_PICK_RESIZE_TYPE = (1 << 3),
	NEMOSHOW_VIEW_PICK_ALL_TYPE = NEMOSHOW_VIEW_PICK_ROTATE_TYPE | NEMOSHOW_VIEW_PICK_SCALE_TYPE | NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE | NEMOSHOW_VIEW_PICK_RESIZE_TYPE
} NemoShowViewPickType;

struct showcontext {
	struct nemotool *tool;

	struct nemotimer *timer;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;

	int32_t width, height;

	int has_framelog;
};

#define NEMOSHOW_AT(show, at)			(((struct showcontext *)nemoshow_get_context(show))->at)

extern struct nemoshow *nemoshow_create_view(struct nemotool *tool, int32_t width, int32_t height);
extern void nemoshow_destroy_view(struct nemoshow *show);
extern void nemoshow_destroy_view_on_idle(struct nemoshow *show);
extern void nemoshow_revoke_view(struct nemoshow *show);

extern void nemoshow_dispatch_frame(struct nemoshow *show);
extern void nemoshow_dispatch_resize(struct nemoshow *show, int32_t width, int32_t height);
extern void nemoshow_dispatch_feedback(struct nemoshow *show);

extern void nemoshow_view_set_layer(struct nemoshow *show, const char *layer);
extern void nemoshow_view_put_layer(struct nemoshow *show);
extern void nemoshow_view_set_fullscreen_type(struct nemoshow *show, const char *type);
extern void nemoshow_view_put_fullscreen_type(struct nemoshow *show, const char *type);
extern void nemoshow_view_set_fullscreen(struct nemoshow *show, const char *id);
extern void nemoshow_view_put_fullscreen(struct nemoshow *show);
extern void nemoshow_view_set_parent(struct nemoshow *show, struct nemocanvas *parent);
extern void nemoshow_view_set_position(struct nemoshow *show, float x, float y);
extern void nemoshow_view_set_rotation(struct nemoshow *show, float r);
extern void nemoshow_view_set_scale(struct nemoshow *show, float sx, float sy);
extern void nemoshow_view_set_pivot(struct nemoshow *show, float px, float py);
extern void nemoshow_view_put_pivot(struct nemoshow *show);
extern void nemoshow_view_set_anchor(struct nemoshow *show, float ax, float ay);
extern void nemoshow_view_set_flag(struct nemoshow *show, float fx, float fy);
extern void nemoshow_view_set_opaque(struct nemoshow *show, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoshow_view_set_min_size(struct nemoshow *show, float width, float height);
extern void nemoshow_view_set_max_size(struct nemoshow *show, float width, float height);
extern void nemoshow_view_set_state(struct nemoshow *show, const char *state);
extern void nemoshow_view_put_state(struct nemoshow *show, const char *state);
extern void nemoshow_view_set_region(struct nemoshow *show, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern void nemoshow_view_put_region(struct nemoshow *show);
extern void nemoshow_view_set_scope(struct nemoshow *show, const char *fmt, ...);
extern void nemoshow_view_put_scope(struct nemoshow *show);
extern void nemoshow_view_set_framerate(struct nemoshow *show, uint32_t framerate);
extern void nemoshow_view_set_tag(struct nemoshow *show, uint32_t tag);
extern void nemoshow_view_set_type(struct nemoshow *show, const char *type);
extern int nemoshow_view_move(struct nemoshow *show, uint32_t serial);
extern int nemoshow_view_pick(struct nemoshow *show, uint32_t serial0, uint32_t serial1, uint32_t type);
extern int nemoshow_view_pick_distant(struct nemoshow *show, void *event, uint32_t type);
extern void nemoshow_view_miss(struct nemoshow *show);
extern void nemoshow_view_focus_to(struct nemoshow *show, uint32_t id);
extern void nemoshow_view_focus_on(struct nemoshow *show, double x, double y);
extern void nemoshow_view_resize(struct nemoshow *show, int32_t width, int32_t height);
extern void nemoshow_view_redraw(struct nemoshow *show);

extern uint32_t nemoshow_view_get_id(struct nemoshow *show);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
