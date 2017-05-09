#ifndef	__PIXMAN_RENDERER_PRIVATE_H__
#define	__PIXMAN_RENDERER_PRIVATE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <wayland-server.h>

#include <renderer.h>
#include <canvas.h>

struct pixmanrenderer {
	struct nemorenderer base;

	struct nemocompz *compz;

	struct wl_signal destroy_signal;
};

struct pixmansurface {
	pixman_image_t *shadow_image;
	pixman_image_t *screen_image;
	void *shadow_buffer;
};

struct pixmancontent {
	struct nemocontent *content;

	struct pixmanrenderer *renderer;

	pixman_image_t *image;
	struct nemobuffer_reference buffer_reference;

	struct wl_listener buffer_destroy_listener;
	struct wl_listener canvas_destroy_listener;
	struct wl_listener renderer_destroy_listener;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
