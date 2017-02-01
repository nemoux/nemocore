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
struct toyzstyle;
struct toyzpath;

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
extern void nemotoyz_style_set_path_effect(struct toyzstyle *style, float segment, float deviation, uint32_t seed);
extern void nemotoyz_style_put_mask_filter(struct toyzstyle *style);
extern void nemotoyz_style_put_image_filter(struct toyzstyle *style);
extern void nemotoyz_style_put_path_effect(struct toyzstyle *style);

extern struct toyzpath *nemotoyz_path_create(void);
extern void nemotoyz_path_destroy(struct toyzpath *path);
extern void nemotoyz_path_clear(struct toyzpath *path);
extern void nemotoyz_path_moveto(struct toyzpath *path, float x, float y);
extern void nemotoyz_path_lineto(struct toyzpath *path, float x, float y);
extern void nemotoyz_path_cubicto(struct toyzpath *path, float x0, float y0, float x1, float y1, float x2, float y2);
extern void nemotoyz_path_arcto(struct toyzpath *path, float x, float y, float w, float h, float from, float to, int needs_moveto);
extern void nemotoyz_path_close(struct toyzpath *path);
extern void nemotoyz_path_cmd(struct toyzpath *path, const char *cmd);
extern void nemotoyz_path_arc(struct toyzpath *path, float x, float y, float w, float h, float from, float to);
extern void nemotoyz_path_append(struct toyzpath *path, struct toyzpath *spath);
extern void nemotoyz_path_translate(struct toyzpath *path, float tx, float ty);
extern void nemotoyz_path_scale(struct toyzpath *path, float sx, float sy);
extern void nemotoyz_path_rotate(struct toyzpath *path, float rz);

extern struct toyzmatrix *nemotoyz_matrix_create(void);
extern void nemotoyz_matrix_destroy(struct toyzmatrix *matrix);
extern void nemotoyz_matrix_set(struct toyzmatrix *matrix, float *ms);
extern void nemotoyz_matrix_identity(struct toyzmatrix *matrix);
extern void nemotoyz_matrix_translate(struct toyzmatrix *matrix, float tx, float ty);
extern void nemotoyz_matrix_scale(struct toyzmatrix *matrix, float sx, float sy);
extern void nemotoyz_matrix_rotate(struct toyzmatrix *matrix, float rz);
extern void nemotoyz_matrix_post_translate(struct toyzmatrix *matrix, float tx, float ty);
extern void nemotoyz_matrix_post_scale(struct toyzmatrix *matrix, float sx, float sy);
extern void nemotoyz_matrix_post_rotate(struct toyzmatrix *matrix, float rz);
extern void nemotoyz_matrix_concat(struct toyzmatrix *matrix, struct toyzmatrix *smatrix);
extern void nemotoyz_matrix_map_point(struct toyzmatrix *matrix, float *x, float *y);
extern void nemotoyz_matrix_map_vector(struct toyzmatrix *matrix, float *x, float *y);
extern void nemotoyz_matrix_map_rectangle(struct toyzmatrix *matrix, float *x, float *y, float *w, float *h);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
