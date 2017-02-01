#ifndef	__NEMOTOYZ_H__
#define	__NEMOTOYZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

typedef enum {
	NEMOTOYZ_CANVAS_32BIT_COLOR = 0,
	NEMOTOYZ_CANVAS_LAST_COLOR
} NemoToyzCanvasColorType;

typedef enum {
	NEMOTOYZ_CANVAS_PREMUL_ALPHA = 0,
	NEMOTOYZ_CANVAS_UNPREMUL_ALPHA = 1,
	NEMOTOYZ_CANVAS_OPAQUE_ALPHA = 2,
	NEMOTOYZ_CANVAS_LAST_ALPHA
} NemoToyzCanvasAlphaType;

struct nemotoyz;

extern struct nemotoyz *nemotoyz_create(void);
extern void nemotoyz_destroy(struct nemotoyz *toyz);

extern int nemotoyz_attach_canvas(struct nemotoyz *toyz, int colortype, int alphatype, void *buffer, int width, int height);
extern void nemotoyz_detach_canvas(struct nemotoyz *toyz);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
