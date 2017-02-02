#ifndef	__NEMOTOYZ_H__
#define	__NEMOTOYZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

typedef enum {
	NEMOTOYZ_CANVAS_NONE_TYPE = 0,
	NEMOTOYZ_CANVAS_BUFFER_TYPE = 1,
	NEMOTOYZ_CANVAS_PICTURE_TYPE = 2,
	NEMOTOYZ_CANVAS_LAST_TYPE
} NemoToyzCanvasType;

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
struct toyzmatrix;
struct toyzregion;
struct toyzpicture;

extern struct nemotoyz *nemotoyz_create(void);
extern void nemotoyz_destroy(struct nemotoyz *toyz);
extern int nemotoyz_attach_buffer(struct nemotoyz *toyz, int colortype, int alphatype, void *buffer, int width, int height);
extern void nemotoyz_detach_buffer(struct nemotoyz *toyz);
extern int nemotoyz_attach_picture(struct nemotoyz *toyz, struct toyzpicture *picture, int width, int height);
extern void nemotoyz_detach_picture(struct nemotoyz *toyz, struct toyzpicture *picture);

extern int nemotoyz_load_image(struct nemotoyz *toyz, const char *url);

extern void nemotoyz_save(struct nemotoyz *toyz);
extern void nemotoyz_save_with_alpha(struct nemotoyz *toyz, float a);
extern void nemotoyz_restore(struct nemotoyz *toyz);
extern void nemotoyz_clear(struct nemotoyz *toyz);
extern void nemotoyz_clear_color(struct nemotoyz *toyz, float r, float g, float b, float a);
extern void nemotoyz_identity(struct nemotoyz *toyz);
extern void nemotoyz_translate(struct nemotoyz *toyz, float tx, float ty);
extern void nemotoyz_scale(struct nemotoyz *toyz, float sx, float sy);
extern void nemotoyz_rotate(struct nemotoyz *toyz, float rz);
extern void nemotoyz_concat(struct nemotoyz *toyz, struct toyzmatrix *matrix);
extern void nemotoyz_matrix(struct nemotoyz *toyz, struct toyzmatrix *matrix);
extern void nemotoyz_clip_rectangle(struct nemotoyz *toyz, float x, float y, float w, float h);
extern void nemotoyz_clip_region(struct nemotoyz *toyz, struct toyzregion *region);
extern void nemotoyz_clip_path(struct nemotoyz *toyz, struct toyzpath *path);
extern void nemotoyz_draw_line(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h);
extern void nemotoyz_draw_rect(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h);
extern void nemotoyz_draw_round_rect(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h, float ox, float oy);
extern void nemotoyz_draw_circle(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float r);
extern void nemotoyz_draw_arc(struct nemotoyz *toyz, struct toyzstyle *style, float x, float y, float w, float h, float from, float to);
extern void nemotoyz_draw_path(struct nemotoyz *toyz, struct toyzstyle *style, struct toyzpath *path);
extern void nemotoyz_draw_points(struct nemotoyz *toyz, struct toyzstyle *style, int npoints, ...);
extern void nemotoyz_draw_polyline(struct nemotoyz *toyz, struct toyzstyle *style, int npoints, ...);
extern void nemotoyz_draw_polygon(struct nemotoyz *toyz, struct toyzstyle *style, int npoints, ...);
extern void nemotoyz_draw_bitmap(struct nemotoyz *toyz, struct toyzstyle *style, struct nemotoyz *bitmap, float x, float y, float w, float h);
extern void nemotoyz_draw_bitmap_with_alpha(struct nemotoyz *toyz, struct toyzstyle *style, struct nemotoyz *bitmap, float x, float y, float w, float h, float alpha);
extern void nemotoyz_draw_picture(struct nemotoyz *toyz, struct toyzpicture *picture);

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
extern void nemotoyz_style_set_linear_gradient_shader(struct toyzstyle *style, float x0, float y0, float x1, float y1, int tilemode, int noffsets, ...);
extern void nemotoyz_style_set_radial_gradient_shader(struct toyzstyle *style, float x, float y, float radius, int tilemode, int noffsets, ...);
extern void nemotoyz_style_set_bitmap_shader(struct toyzstyle *style, struct nemotoyz *bitmap, int tilemodex, int tilemodey);
extern void nemotoyz_style_put_shader(struct toyzstyle *style);
extern void nemotoyz_style_transform_shader(struct toyzstyle *style, struct toyzmatrix *matrix);

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
extern void nemotoyz_matrix_invert(struct toyzmatrix *matrix, struct toyzmatrix *smatrix);
extern void nemotoyz_matrix_map_point(struct toyzmatrix *matrix, float *x, float *y);
extern void nemotoyz_matrix_map_vector(struct toyzmatrix *matrix, float *x, float *y);
extern void nemotoyz_matrix_map_rectangle(struct toyzmatrix *matrix, float *x, float *y, float *w, float *h);

extern struct toyzregion *nemotoyz_region_create(void);
extern void nemotoyz_region_destroy(struct toyzregion *region);
extern void nemotoyz_region_clear(struct toyzregion *region);
extern void nemotoyz_region_set_rectangle(struct toyzregion *region, float x, float y, float w, float h);
extern void nemotoyz_region_intersect(struct toyzregion *region, float x, float y, float w, float h);
extern void nemotoyz_region_union(struct toyzregion *region, float x, float y, float w, float h);
extern int nemotoyz_region_check_intersection(struct toyzregion *region, float x, float y, float w, float h);

extern struct toyzpicture *nemotoyz_picture_create(void);
extern void nemotoyz_picture_destroy(struct toyzpicture *picture);
extern void nemotoyz_picture_save(struct toyzpicture *picture, const char *url);
extern void nemotoyz_picture_load(struct toyzpicture *picture, const char *url);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
