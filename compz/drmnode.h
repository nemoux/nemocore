#ifndef	__DRM_NODE_H__
#define	__DRM_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <time.h>
#include <pixman.h>

#include <compz.h>
#include <canvas.h>
#include <screen.h>
#include <renderer.h>

struct drmmode {
	struct nemomode base;

	drmModeModeInfo modeinfo;
};

struct drmframe {
	uint32_t id, stride, handle, size;
	int fd;
	int is_client_buffer;
	struct nemobuffer_reference buffer_reference;

	struct gbm_bo *bo;

	void *map;
};

struct drmscreen {
	struct nemoscreen base;

	struct drmnode *node;

	uint32_t crtc_id;
	uint32_t connector_id;
	uint32_t pipe;

	int pageflip_pending;

	drmModeCrtcPtr orig_crtc;
	drmModePropertyPtr dpms_prop;

	uint32_t format;

	struct gbm_surface *surface;

	struct drmframe *current, *next;

	struct drmframe *dumbs[2];
	pixman_image_t *images[2];
	int current_image;

	pixman_region32_t damage;

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	struct wl_event_source *pageflip_timer;
#endif
};

struct drmnode {
	struct rendernode base;

	int fd;
	int has_master;
	struct wl_event_source *source;
	char *devnode;

	clockid_t clock;

	uint32_t *crtcs;
	int ncrtcs;

	uint32_t crtc_allocator;
	uint32_t connector_allocator;

	uint32_t min_width, max_width;
	uint32_t min_height, max_height;
	int no_addfb2;

	struct gbm_device *gbm;
	uint32_t format;

	struct wl_listener session_listener;

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	uint32_t pageflip_timeout;
#endif
};

extern struct drmnode *drm_create_node(struct nemocompz *compz, uint32_t nodeid, const char *path, int fd);
extern void drm_destroy_node(struct drmnode *node);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
