#ifndef	__NEMOTOYZ_H__
#define	__NEMOTOYZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

typedef enum {
	NEMOTOYZ_CANVAS_RGBA_COLOR = 0,
	NEMOTOYZ_CANVAS_LAST_COLOR
} NemoToyzCanvasColorType;

typedef enum {
	NEMOTOYZ_CANVAS_PREMUL_ALPHA = 0,
	NEMOTOYZ_CANVAS_UNPREMUL_ALPHA = 1,
	NEMOTOYZ_CANVAS_OPAQUE_ALPHA = 2,
	NEMOTOYZ_CANVAS_LAST_ALPHA
} NemoToyzCanvasAlphaType;

typedef enum {
	NEMOTOYZ_STYLE_FILL_TYPE = 0,
	NEMOTOYZ_STYLE_STROKE_TYPE = 1,
	NEMOTOYZ_STYLE_STROKE_AND_FILL_TYPE = 2,
	NEMOTOYZ_STYLE_LAST_TYPE
} NemoToyzStyleType;

typedef enum {
	NEMOTOYZ_STYLE_STROKE_BUTT_CAP = 0,
	NEMOTOYZ_STYLE_STROKE_ROUND_CAP = 1,
	NEMOTOYZ_STYLE_STROKE_SQUARE_CAP = 2,
	NEMOTOYZ_STYLE_STROKE_LAST_CAP
} NemoToyzStyleStrokeCap;

typedef enum {
	NEMOTOYZ_STYLE_STROKE_MITER_JOIN = 0,
	NEMOTOYZ_STYLE_STROKE_ROUND_JOIN = 1,
	NEMOTOYZ_STYLE_STROKE_BEVEL_JOIN = 2,
	NEMOTOYZ_STYLE_STROKE_LAST_JOIN
} NemoToyzStyleStrokeJoin;

struct nemotoyz;

extern struct nemotoyz *nemotoyz_create(void);
extern void nemotoyz_destroy(struct nemotoyz *toyz);

extern int nemotoyz_attach_canvas(struct nemotoyz *toyz, int colortype, int alphatype, void *buffer, int width, int height);
extern void nemotoyz_detach_canvas(struct nemotoyz *toyz);

extern struct toyzstyle *nemotoyz_style_create(void);
extern void nemotoyz_style_destroy(struct toyzstyle *style);

extern void nemotoyz_style_set_type(struct toyzstyle *style, int type);
extern void nemotoyz_style_set_color(struct toyzstyle *style, float r, float g, float b, float a);
extern void nemotoyz_style_set_stroke_width(struct toyzstyle *style, float w);
extern void nemotoyz_style_set_stroke_cap(struct toyzstyle *style, int cap);
extern void nemotoyz_style_set_stroke_join(struct toyzstyle *style, int join);
extern void nemotoyz_style_set_anti_alias(struct toyzstyle *style, int use_antialias);

extern void nemotoyz_style_set_blur_filter(struct toyzstyle *style, int type, int quality, float r);
extern void nemotoyz_style_set_emboss_filter(struct toyzstyle *style, float x, float y, float z, float r, float ambient, float specular);
extern void nemotoyz_style_set_shadow_filter(struct toyzstyle *style, int mode, float dx, float dy, float sx, float sy, float r, float g, float b, float a);
extern void nemotoyz_style_put_mask_filter(struct toyzstyle *style);
extern void nemotoyz_style_put_image_filter(struct toyzstyle *style);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
