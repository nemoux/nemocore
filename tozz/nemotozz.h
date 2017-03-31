#ifndef	__NEMOTOZZ_H__
#define	__NEMOTOZZ_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

typedef enum {
	NEMOTOZZ_CANVAS_NONE_TYPE = 0,
	NEMOTOZZ_CANVAS_BUFFER_TYPE = 1,
	NEMOTOZZ_CANVAS_PICTURE_TYPE = 2,
	NEMOTOZZ_CANVAS_LAST_TYPE
} NemoTozzCanvasType;

typedef enum {
	NEMOTOZZ_CANVAS_RGBA_COLOR = 0,
	NEMOTOZZ_CANVAS_LAST_COLOR
} NemoTozzCanvasColorType;

typedef enum {
	NEMOTOZZ_CANVAS_PREMUL_ALPHA = 0,
	NEMOTOZZ_CANVAS_UNPREMUL_ALPHA = 1,
	NEMOTOZZ_CANVAS_OPAQUE_ALPHA = 2,
	NEMOTOZZ_CANVAS_LAST_ALPHA
} NemoTozzCanvasAlphaType;

typedef enum {
	NEMOTOZZ_STYLE_FILL_TYPE = 0,
	NEMOTOZZ_STYLE_STROKE_TYPE = 1,
	NEMOTOZZ_STYLE_STROKE_AND_FILL_TYPE = 2,
	NEMOTOZZ_STYLE_LAST_TYPE
} NemoTozzStyleType;

typedef enum {
	NEMOTOZZ_STYLE_STROKE_BUTT_CAP = 0,
	NEMOTOZZ_STYLE_STROKE_ROUND_CAP = 1,
	NEMOTOZZ_STYLE_STROKE_SQUARE_CAP = 2,
	NEMOTOZZ_STYLE_STROKE_LAST_CAP
} NemoTozzStyleStrokeCap;

typedef enum {
	NEMOTOZZ_STYLE_STROKE_MITER_JOIN = 0,
	NEMOTOZZ_STYLE_STROKE_ROUND_JOIN = 1,
	NEMOTOZZ_STYLE_STROKE_BEVEL_JOIN = 2,
	NEMOTOZZ_STYLE_STROKE_LAST_JOIN
} NemoTozzStyleStrokeJoin;

struct nemotozz;
struct tozzstyle;
struct tozzpath;
struct tozzmatrix;
struct tozzregion;
struct tozzpicture;

extern struct nemotozz *nemotozz_create(void);
extern void nemotozz_destroy(struct nemotozz *tozz);
extern int nemotozz_attach_buffer(struct nemotozz *tozz, int colortype, int alphatype, void *buffer, int width, int height);
extern void nemotozz_detach_buffer(struct nemotozz *tozz);
extern int nemotozz_attach_picture(struct nemotozz *tozz, struct tozzpicture *picture, int width, int height);
extern void nemotozz_detach_picture(struct nemotozz *tozz, struct tozzpicture *picture);

extern int nemotozz_load_image(struct nemotozz *tozz, const char *url);

extern void nemotozz_save(struct nemotozz *tozz);
extern void nemotozz_save_with_alpha(struct nemotozz *tozz, float a);
extern void nemotozz_restore(struct nemotozz *tozz);
extern void nemotozz_clear(struct nemotozz *tozz);
extern void nemotozz_clear_color(struct nemotozz *tozz, float r, float g, float b, float a);
extern void nemotozz_identity(struct nemotozz *tozz);
extern void nemotozz_translate(struct nemotozz *tozz, float tx, float ty);
extern void nemotozz_scale(struct nemotozz *tozz, float sx, float sy);
extern void nemotozz_rotate(struct nemotozz *tozz, float rz);
extern void nemotozz_concat(struct nemotozz *tozz, struct tozzmatrix *matrix);
extern void nemotozz_matrix(struct nemotozz *tozz, struct tozzmatrix *matrix);
extern void nemotozz_clip_rectangle(struct nemotozz *tozz, float x, float y, float w, float h);
extern void nemotozz_clip_region(struct nemotozz *tozz, struct tozzregion *region);
extern void nemotozz_clip_path(struct nemotozz *tozz, struct tozzpath *path);
extern void nemotozz_draw_line(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h);
extern void nemotozz_draw_rect(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h);
extern void nemotozz_draw_round_rect(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h, float ox, float oy);
extern void nemotozz_draw_circle(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float r);
extern void nemotozz_draw_arc(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, float w, float h, float from, float to);
extern void nemotozz_draw_path(struct nemotozz *tozz, struct tozzstyle *style, struct tozzpath *path);
extern void nemotozz_draw_points(struct nemotozz *tozz, struct tozzstyle *style, int npoints, ...);
extern void nemotozz_draw_polyline(struct nemotozz *tozz, struct tozzstyle *style, int npoints, ...);
extern void nemotozz_draw_polygon(struct nemotozz *tozz, struct tozzstyle *style, int npoints, ...);
extern void nemotozz_draw_text(struct nemotozz *tozz, struct tozzstyle *style, float x, float y, const char *text);
extern void nemotozz_draw_text_on_path(struct nemotozz *tozz, struct tozzstyle *style, struct tozzpath *path, const char *text);
extern void nemotozz_draw_bitmap(struct nemotozz *tozz, struct tozzstyle *style, struct nemotozz *bitmap, float x, float y, float w, float h);
extern void nemotozz_draw_bitmap_with_alpha(struct nemotozz *tozz, struct tozzstyle *style, struct nemotozz *bitmap, float x, float y, float w, float h, float alpha);
extern void nemotozz_draw_picture(struct nemotozz *tozz, struct tozzpicture *picture);

extern struct tozzstyle *nemotozz_style_create(void);
extern void nemotozz_style_destroy(struct tozzstyle *style);
extern void nemotozz_style_set_type(struct tozzstyle *style, int type);
extern void nemotozz_style_set_color(struct tozzstyle *style, float r, float g, float b, float a);
extern void nemotozz_style_set_stroke_width(struct tozzstyle *style, float w);
extern void nemotozz_style_set_stroke_cap(struct tozzstyle *style, int cap);
extern void nemotozz_style_set_stroke_join(struct tozzstyle *style, int join);
extern void nemotozz_style_set_anti_alias(struct tozzstyle *style, int use_antialias);
extern void nemotozz_style_set_blur_filter(struct tozzstyle *style, int type, int quality, float r);
extern void nemotozz_style_set_emboss_filter(struct tozzstyle *style, float x, float y, float z, float r, float ambient, float specular);
extern void nemotozz_style_set_shadow_filter(struct tozzstyle *style, int mode, float dx, float dy, float sx, float sy, float r, float g, float b, float a);
extern void nemotozz_style_set_path_effect(struct tozzstyle *style, float segment, float deviation, uint32_t seed);
extern void nemotozz_style_set_dash_effect(struct tozzstyle *style, int *dashes, int count);
extern void nemotozz_style_put_mask_filter(struct tozzstyle *style);
extern void nemotozz_style_put_image_filter(struct tozzstyle *style);
extern void nemotozz_style_put_path_effect(struct tozzstyle *style);
extern void nemotozz_style_set_linear_gradient_shader(struct tozzstyle *style, float x0, float y0, float x1, float y1, int tilemode, int noffsets, ...);
extern void nemotozz_style_set_radial_gradient_shader(struct tozzstyle *style, float x, float y, float radius, int tilemode, int noffsets, ...);
extern void nemotozz_style_set_bitmap_shader(struct tozzstyle *style, struct nemotozz *bitmap, int tilemodex, int tilemodey);
extern void nemotozz_style_put_shader(struct tozzstyle *style);
extern void nemotozz_style_transform_shader(struct tozzstyle *style, struct tozzmatrix *matrix);
extern void nemotozz_style_load_font(struct tozzstyle *style, const char *path, int index);
extern void nemotozz_style_load_fontconfig(struct tozzstyle *style, const char *fontfamily, const char *fontstyle);
extern void nemotozz_style_set_font_size(struct tozzstyle *style, float fontsize);
extern float nemotozz_style_get_text_height(struct tozzstyle *style);
extern float nemotozz_style_get_text_width(struct tozzstyle *style, const char *text, int length);

extern struct tozzpath *nemotozz_path_create(void);
extern void nemotozz_path_destroy(struct tozzpath *path);
extern void nemotozz_path_clear(struct tozzpath *path);
extern void nemotozz_path_moveto(struct tozzpath *path, float x, float y);
extern void nemotozz_path_lineto(struct tozzpath *path, float x, float y);
extern void nemotozz_path_cubicto(struct tozzpath *path, float x0, float y0, float x1, float y1, float x2, float y2);
extern void nemotozz_path_arcto(struct tozzpath *path, float x, float y, float w, float h, float from, float to, int needs_moveto);
extern void nemotozz_path_close(struct tozzpath *path);
extern void nemotozz_path_arc(struct tozzpath *path, float x, float y, float w, float h, float from, float to);
extern void nemotozz_path_text(struct tozzpath *path, struct tozzstyle *style, float x, float y, const char *text);
extern void nemotozz_path_cmd(struct tozzpath *path, const char *cmd);
extern void nemotozz_path_svg(struct tozzpath *path, const char *url, float x, float y, float w, float h);
extern void nemotozz_path_append(struct tozzpath *path, struct tozzpath *spath);
extern void nemotozz_path_translate(struct tozzpath *path, float tx, float ty);
extern void nemotozz_path_scale(struct tozzpath *path, float sx, float sy);
extern void nemotozz_path_rotate(struct tozzpath *path, float rz);
extern void nemotozz_path_bounds(struct tozzpath *path, float *x, float *y, float *w, float *h);
extern void nemotozz_path_measure(struct tozzpath *path);
extern float nemotozz_path_length(struct tozzpath *path);
extern int nemotozz_path_position(struct tozzpath *path, float t, float *px, float *py, float *tx, float *ty);
extern void nemotozz_path_segment(struct tozzpath *path, float from, float to, struct tozzpath *spath);
extern void nemotozz_path_dump(struct tozzpath *path, FILE *out);

extern struct tozzmatrix *nemotozz_matrix_create(void);
extern void nemotozz_matrix_destroy(struct tozzmatrix *matrix);
extern void nemotozz_matrix_set(struct tozzmatrix *matrix, float *ms);
extern void nemotozz_matrix_identity(struct tozzmatrix *matrix);
extern void nemotozz_matrix_translate(struct tozzmatrix *matrix, float tx, float ty);
extern void nemotozz_matrix_scale(struct tozzmatrix *matrix, float sx, float sy);
extern void nemotozz_matrix_rotate(struct tozzmatrix *matrix, float rz);
extern void nemotozz_matrix_post_translate(struct tozzmatrix *matrix, float tx, float ty);
extern void nemotozz_matrix_post_scale(struct tozzmatrix *matrix, float sx, float sy);
extern void nemotozz_matrix_post_rotate(struct tozzmatrix *matrix, float rz);
extern void nemotozz_matrix_concat(struct tozzmatrix *matrix, struct tozzmatrix *smatrix);
extern void nemotozz_matrix_invert(struct tozzmatrix *matrix, struct tozzmatrix *smatrix);
extern void nemotozz_matrix_map_point(struct tozzmatrix *matrix, float *x, float *y);
extern void nemotozz_matrix_map_vector(struct tozzmatrix *matrix, float *x, float *y);
extern void nemotozz_matrix_map_rectangle(struct tozzmatrix *matrix, float *x, float *y, float *w, float *h);

extern struct tozzregion *nemotozz_region_create(void);
extern void nemotozz_region_destroy(struct tozzregion *region);
extern void nemotozz_region_clear(struct tozzregion *region);
extern void nemotozz_region_set_rectangle(struct tozzregion *region, float x, float y, float w, float h);
extern void nemotozz_region_intersect(struct tozzregion *region, float x, float y, float w, float h);
extern void nemotozz_region_union(struct tozzregion *region, float x, float y, float w, float h);
extern int nemotozz_region_check_intersection(struct tozzregion *region, float x, float y, float w, float h);

extern struct tozzpicture *nemotozz_picture_create(void);
extern void nemotozz_picture_destroy(struct tozzpicture *picture);
extern void nemotozz_picture_save(struct tozzpicture *picture, const char *url);
extern void nemotozz_picture_load(struct tozzpicture *picture, const char *url);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
