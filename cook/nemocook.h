#ifndef __NEMOCOOK_H__
#define __NEMOCOOK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <cookshader.h>
#include <cooktex.h>
#include <cookpoly.h>
#include <cooktrans.h>
#include <cookegl.h>
#include <cookfbo.h>

struct nemocook;

typedef int (*nemocook_backend_prerender_t)(struct nemocook *cook);
typedef int (*nemocook_backend_postrender_t)(struct nemocook *cook);
typedef int (*nemocook_backend_resize_t)(struct nemocook *cook, int width, int height);
typedef void (*nemocook_backend_finish_t)(struct nemocook *cook);

struct nemocook {
	nemocook_backend_prerender_t backend_prerender;
	nemocook_backend_postrender_t backend_postrender;
	nemocook_backend_resize_t backend_resize;
	nemocook_backend_finish_t backend_finish;
	void *backend;

	uint32_t width, height;
};

extern struct nemocook *nemocook_create(void);
extern void nemocook_destroy(struct nemocook *cook);

extern void nemocook_set_size(struct nemocook *cook, uint32_t width, uint32_t height);

static inline int nemocook_backend_prerender(struct nemocook *cook)
{
	return cook->backend_prerender(cook);
}

static inline int nemocook_backend_postrender(struct nemocook *cook)
{
	return cook->backend_postrender(cook);
}

static inline int nemocook_backend_resize(struct nemocook *cook, int width, int height)
{
	return cook->backend_resize(cook, width, height);
}

static inline void nemocook_backend_finish(struct nemocook *cook)
{
	cook->backend_finish(cook);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
