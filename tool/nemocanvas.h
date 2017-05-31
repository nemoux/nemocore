#ifndef __NEMOTOOL_CANVAS_H__
#define	__NEMOTOOL_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <pixman.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemolist.h>
#include <nemolistener.h>

struct nemocanvas;

typedef int (*nemocanvas_dispatch_event_t)(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event);
typedef void (*nemocanvas_dispatch_resize_t)(struct nemocanvas *canvas, int32_t width, int32_t height);
typedef void (*nemocanvas_dispatch_transform_t)(struct nemocanvas *canvas, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*nemocanvas_dispatch_layer_t)(struct nemocanvas *canvas, int32_t visible);
typedef void (*nemocanvas_dispatch_fullscreen_t)(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*nemocanvas_dispatch_frame_t)(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs);
typedef void (*nemocanvas_dispatch_discard_t)(struct nemocanvas *canvas);
typedef void (*nemocanvas_dispatch_screen_t)(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mmwidth, int32_t mmheight, int left);
typedef int (*nemocanvas_dispatch_destroy_t)(struct nemocanvas *canvas);

struct nemobuffer {
	struct wl_buffer *buffer;
	char *shm_data;
	int shm_size;
	int busy;

	int width, height;
};

struct nemocanvas {
	struct nemotool *tool;

	struct wl_surface *surface;
	struct wl_subsurface *subsurface;
	struct nemo_surface *nemo_surface;

	struct presentation_feedback *feedback;
	uint32_t framemsecs;
	uint32_t framerate;

	struct nemocanvas *parent;

	int width, height;
	pixman_region32_t damage;

	struct nemobuffer *buffers;
	struct nemobuffer *buffer;
	int nbuffers;

	nemocanvas_dispatch_event_t dispatch_event;
	nemocanvas_dispatch_resize_t dispatch_resize;
	nemocanvas_dispatch_transform_t dispatch_transform;
	nemocanvas_dispatch_layer_t dispatch_layer;
	nemocanvas_dispatch_fullscreen_t dispatch_fullscreen;
	nemocanvas_dispatch_frame_t dispatch_frame;
	nemocanvas_dispatch_discard_t dispatch_discard;
	nemocanvas_dispatch_screen_t dispatch_screen;
	nemocanvas_dispatch_destroy_t dispatch_destroy;

	struct nemosignal destroy_signal;

	void *userdata;
};

#define	NEMOCANVAS_DAMAGE(canvas)			(&canvas->damage)

extern struct nemocanvas *nemocanvas_create(struct nemotool *tool);
extern void nemocanvas_destroy(struct nemocanvas *canvas);

extern int nemocanvas_init(struct nemocanvas *canvas, struct nemotool *tool);
extern void nemocanvas_exit(struct nemocanvas *canvas);

extern int nemocanvas_ready(struct nemocanvas *canvas);
extern void nemocanvas_damage(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemocanvas_damage_region(struct nemocanvas *canvas, pixman_region32_t *region);
extern void nemocanvas_commit(struct nemocanvas *canvas);
extern void nemocanvas_opaque(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height);

extern pixman_image_t *nemocanvas_get_pixman_image(struct nemocanvas *canvas);

extern int nemocanvas_set_buffer_max(struct nemocanvas *canvas, int nbuffers);

extern void nemocanvas_set_tag(struct nemocanvas *canvas, uint32_t tag);
extern void nemocanvas_set_type(struct nemocanvas *canvas, const char *type);
extern void nemocanvas_set_uuid(struct nemocanvas *canvas, const char *uuid);
extern void nemocanvas_set_state(struct nemocanvas *canvas, const char *state);
extern void nemocanvas_put_state(struct nemocanvas *canvas, const char *state);
extern void nemocanvas_set_size(struct nemocanvas *canvas, int32_t width, int32_t height);
extern void nemocanvas_set_min_size(struct nemocanvas *canvas, int32_t width, int32_t height);
extern void nemocanvas_set_max_size(struct nemocanvas *canvas, int32_t width, int32_t height);
extern void nemocanvas_set_position(struct nemocanvas *canvas, float x, float y);
extern void nemocanvas_set_rotation(struct nemocanvas *canvas, float r);
extern void nemocanvas_set_scale(struct nemocanvas *canvas, float sx, float sy);
extern void nemocanvas_set_pivot(struct nemocanvas *canvas, int px, int py);
extern void nemocanvas_set_anchor(struct nemocanvas *canvas, float ax, float ay);
extern void nemocanvas_set_flag(struct nemocanvas *canvas, float fx, float fy);
extern void nemocanvas_set_layer(struct nemocanvas *canvas, const char *type);
extern void nemocanvas_set_parent(struct nemocanvas *canvas, struct nemocanvas *parent);
extern void nemocanvas_set_region(struct nemocanvas *canvas, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern void nemocanvas_put_region(struct nemocanvas *canvas);
extern void nemocanvas_set_scope(struct nemocanvas *canvas, const char *cmds);
extern void nemocanvas_put_scope(struct nemocanvas *canvas);
extern void nemocanvas_set_fullscreen_type(struct nemocanvas *canvas, const char *type);
extern void nemocanvas_put_fullscreen_type(struct nemocanvas *canvas, const char *type);
extern void nemocanvas_set_fullscreen_target(struct nemocanvas *canvas, const char *id);
extern void nemocanvas_put_fullscreen_target(struct nemocanvas *canvas);
extern void nemocanvas_set_fullscreen(struct nemocanvas *canvas, const char *id);
extern void nemocanvas_put_fullscreen(struct nemocanvas *canvas);
extern void nemocanvas_move(struct nemocanvas *canvas, uint32_t serial);
extern void nemocanvas_pick(struct nemocanvas *canvas, uint32_t serial0, uint32_t serial1, const char *type);
extern void nemocanvas_miss(struct nemocanvas *canvas);
extern void nemocanvas_focus_to(struct nemocanvas *canvas, const char *uuid);
extern void nemocanvas_focus_on(struct nemocanvas *canvas, double x, double y);
extern void nemocanvas_update(struct nemocanvas *canvas, uint32_t serial);

extern void nemocanvas_set_nemosurface(struct nemocanvas *canvas, const char *type);

extern int nemocanvas_set_subsurface(struct nemocanvas *canvas, struct nemocanvas *parent);
extern void nemocanvas_set_subsurface_position(struct nemocanvas *canvas, int32_t x, int32_t y);
extern void nemocanvas_set_subsurface_sync(struct nemocanvas *canvas);
extern void nemocanvas_set_subsurface_desync(struct nemocanvas *canvas);

extern void nemocanvas_set_dispatch_event(struct nemocanvas *canvas, nemocanvas_dispatch_event_t dispatch);
extern void nemocanvas_set_dispatch_resize(struct nemocanvas *canvas, nemocanvas_dispatch_resize_t dispatch);
extern void nemocanvas_set_dispatch_transform(struct nemocanvas *canvas, nemocanvas_dispatch_transform_t dispatch);
extern void nemocanvas_set_dispatch_layer(struct nemocanvas *canvas, nemocanvas_dispatch_layer_t dispatch);
extern void nemocanvas_set_dispatch_fullscreen(struct nemocanvas *canvas, nemocanvas_dispatch_fullscreen_t dispatch);
extern void nemocanvas_set_dispatch_frame(struct nemocanvas *canvas, nemocanvas_dispatch_frame_t dispatch);
extern void nemocanvas_set_dispatch_discard(struct nemocanvas *canvas, nemocanvas_dispatch_discard_t dispatch);
extern void nemocanvas_set_dispatch_screen(struct nemocanvas *canvas, nemocanvas_dispatch_screen_t dispatch);
extern void nemocanvas_set_dispatch_destroy(struct nemocanvas *canvas, nemocanvas_dispatch_destroy_t dispatch);

extern void nemocanvas_set_framerate(struct nemocanvas *canvas, uint32_t framerate);

extern void nemocanvas_dispatch_feedback(struct nemocanvas *canvas);
extern void nemocanvas_terminate_feedback(struct nemocanvas *canvas);

extern void nemocanvas_dispatch_frame(struct nemocanvas *canvas);

extern void nemocanvas_dispatch_resize(struct nemocanvas *canvas, int32_t width, int32_t height);
extern int nemocanvas_dispatch_destroy(struct nemocanvas *canvas);

static inline struct nemotool *nemocanvas_get_tool(struct nemocanvas *canvas)
{
	return canvas->tool;
}

static inline struct nemocanvas *nemocanvas_get_parent(struct nemocanvas *canvas)
{
	return canvas->parent;
}

static inline struct wl_surface *nemocanvas_get_surface(struct nemocanvas *canvas)
{
	return canvas->surface;
}

static inline char *nemocanvas_get_data(struct nemocanvas *canvas)
{
	return canvas->buffer == NULL ? NULL : canvas->buffer->shm_data;
}

static inline int nemocanvas_get_width(struct nemocanvas *canvas)
{
	return canvas->width;
}

static inline int nemocanvas_get_height(struct nemocanvas *canvas)
{
	return canvas->height;
}

static inline int nemocanvas_get_stride(struct nemocanvas *canvas)
{
	return canvas->width * 4;
}

static inline void nemocanvas_set_userdata(struct nemocanvas *canvas, void *data)
{
	canvas->userdata = data;
}

static inline void *nemocanvas_get_userdata(struct nemocanvas *canvas)
{
	return canvas->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
