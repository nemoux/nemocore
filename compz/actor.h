#ifndef	__NEMO_ACTOR_H__
#define	__NEMO_ACTOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <content.h>
#include <event.h>

struct nemoview;
struct nemoscreen;

typedef int (*nemoactor_dispatch_event_t)(struct nemoactor *actor, uint32_t type, struct nemoevent *event);
typedef int (*nemoactor_dispatch_pick_t)(struct nemoactor *actor, float x, float y);
typedef int (*nemoactor_dispatch_resize_t)(struct nemoactor *actor, int32_t width, int32_t height);
typedef void (*nemoactor_dispatch_output_t)(struct nemoactor *actor, uint32_t node_mask, uint32_t screen_mask);
typedef void (*nemoactor_dispatch_transform_t)(struct nemoactor *actor, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*nemoactor_dispatch_layer_t)(struct nemoactor *actor, int32_t visible);
typedef void (*nemoactor_dispatch_fullscreen_t)(struct nemoactor *actor, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);
typedef void (*nemoactor_dispatch_frame_t)(struct nemoactor *actor, uint32_t msecs);
typedef int (*nemoactor_dispatch_destroy_t)(struct nemoactor *actor);

struct nemoactor {
	struct nemocontent base;

	struct nemocompz *compz;

	struct nemoview *view;

	struct wl_signal destroy_signal;
	struct wl_signal ungrab_signal;

	struct wl_list link;

	int newly_attached;

	pixman_image_t *image;
	void *data;

	uint32_t min_width, min_height;
	uint32_t max_width, max_height;

	GLuint texture;

	nemoactor_dispatch_event_t dispatch_event;
	nemoactor_dispatch_pick_t dispatch_pick;
	nemoactor_dispatch_resize_t dispatch_resize;
	nemoactor_dispatch_output_t dispatch_output;
	nemoactor_dispatch_transform_t dispatch_transform;
	nemoactor_dispatch_layer_t dispatch_layer;
	nemoactor_dispatch_fullscreen_t dispatch_fullscreen;
	nemoactor_dispatch_frame_t dispatch_frame;
	nemoactor_dispatch_destroy_t dispatch_destroy;

	struct {
		float ax, ay;
	} scale;

	struct wl_list frame_link;

	void *context;
};

extern struct nemoactor *nemoactor_create_pixman(struct nemocompz *compz, int width, int height);
extern void nemoactor_destroy(struct nemoactor *actor);
extern int nemoactor_resize_pixman(struct nemoactor *actor, int width, int height);

extern struct nemoactor *nemoactor_create_gl(struct nemocompz *compz, int width, int height);
extern int nemoactor_resize_gl(struct nemoactor *actor, int width, int height);

extern void nemoactor_schedule_repaint(struct nemoactor *actor);
extern void nemoactor_damage_dirty(struct nemoactor *actor);
extern void nemoactor_damage_below(struct nemoactor *actor);
extern void nemoactor_damage(struct nemoactor *actor, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoactor_damage_region(struct nemoactor *actor, pixman_region32_t *region);
extern void nemoactor_flush_damage(struct nemoactor *actor);

extern void nemoactor_set_dispatch_event(struct nemoactor *actor, nemoactor_dispatch_event_t dispatch);
extern void nemoactor_set_dispatch_pick(struct nemoactor *actor, nemoactor_dispatch_pick_t dispatch);
extern void nemoactor_set_dispatch_resize(struct nemoactor *actor, nemoactor_dispatch_resize_t dispatch);
extern void nemoactor_set_dispatch_output(struct nemoactor *actor, nemoactor_dispatch_output_t dispatch);
extern void nemoactor_set_dispatch_transform(struct nemoactor *actor, nemoactor_dispatch_transform_t dispatch);
extern void nemoactor_set_dispatch_layer(struct nemoactor *actor, nemoactor_dispatch_layer_t dispatch);
extern void nemoactor_set_dispatch_fullscreen(struct nemoactor *actor, nemoactor_dispatch_fullscreen_t dispatch);
extern void nemoactor_set_dispatch_frame(struct nemoactor *actor, nemoactor_dispatch_frame_t dispatch);
extern void nemoactor_set_dispatch_destroy(struct nemoactor *actor, nemoactor_dispatch_destroy_t dispatch);

extern int nemoactor_dispatch_event(struct nemoactor *actor, uint32_t type, struct nemoevent *event);
extern int nemoactor_dispatch_pick(struct nemoactor *actor, float x, float y);
extern int nemoactor_dispatch_resize(struct nemoactor *actor, int32_t width, int32_t height);
extern void nemoactor_dispatch_output(struct nemoactor *actor, uint32_t node_mask, uint32_t screen_mask);
extern void nemoactor_dispatch_transform(struct nemoactor *actor, int visible, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoactor_dispatch_layer(struct nemoactor *actor, int visible);
extern void nemoactor_dispatch_fullscreen(struct nemoactor *actor, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoactor_dispatch_frame(struct nemoactor *actor);
extern void nemoactor_dispatch_feedback(struct nemoactor *actor);
extern void nemoactor_terminate_feedback(struct nemoactor *actor);
extern int nemoactor_dispatch_destroy(struct nemoactor *actor);

extern void nemoactor_set_min_size(struct nemoactor *actor, uint32_t width, uint32_t height);
extern void nemoactor_set_max_size(struct nemoactor *actor, uint32_t width, uint32_t height);

static inline void nemoactor_set_context(struct nemoactor *actor, void *context)
{
	actor->context = context;
}

static inline void *nemoactor_get_context(struct nemoactor *actor)
{
	return actor->context;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
