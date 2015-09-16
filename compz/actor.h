#ifndef	__NEMO_ACTOR_H__
#define	__NEMO_ACTOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>

#ifdef NEMOUX_WITH_EGL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#include <content.h>

struct nemoview;
struct nemoscreen;

typedef void (*nemoactor_dispatch_resize_t)(struct nemoactor *actor, int32_t width, int32_t height);
typedef void (*nemoactor_dispatch_frame_t)(struct nemoactor *actor, uint32_t msecs);

struct nemoactor {
	struct nemocontent base;

	struct nemocompz *compz;

	struct nemoview *view;

	struct wl_signal destroy_signal;
	struct wl_signal ungrab_signal;

	struct wl_list link;

	int grabbed;

	int newly_attached;

	pixman_image_t *image;
	void *data;

	uint32_t min_width, min_height;
	uint32_t max_width, max_height;

#ifdef NEMOUX_WITH_EGL
	GLuint texture;
#endif

	nemoactor_dispatch_resize_t dispatch_resize;
	nemoactor_dispatch_frame_t dispatch_frame;

	struct wl_list frame_link;

	void *context;
};

extern struct nemoactor *nemoactor_create_pixman(struct nemocompz *compz, int width, int height);
extern void nemoactor_destroy(struct nemoactor *actor);
extern int nemoactor_resize_pixman(struct nemoactor *actor, int width, int height);

#ifdef NEMOUX_WITH_EGL
extern struct nemoactor *nemoactor_create_gl(struct nemocompz *compz, int width, int height);
extern int nemoactor_resize_gl(struct nemoactor *actor, int width, int height);
#endif

extern void nemoactor_schedule_repaint(struct nemoactor *actor);
extern void nemoactor_damage_dirty(struct nemoactor *actor);
extern void nemoactor_damage(struct nemoactor *actor, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemoactor_damage_region(struct nemoactor *actor, pixman_region32_t *region);
extern void nemoactor_flush_damage(struct nemoactor *actor);

extern void nemoactor_set_dispatch_resize(struct nemoactor *actor, nemoactor_dispatch_resize_t dispatch);
extern void nemoactor_set_dispatch_frame(struct nemoactor *actor, nemoactor_dispatch_frame_t dispatch);

extern void nemoactor_dispatch_resize(struct nemoactor *actor, int32_t width, int32_t height);
extern void nemoactor_dispatch_frame(struct nemoactor *actor);

extern void nemoactor_feedback(struct nemoactor *actor);
extern void nemoactor_feedback_done(struct nemoactor *actor);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
