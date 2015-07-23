#ifndef	__NEMO_PATH_H__
#define	__NEMO_PATH_H__

#include <stdint.h>
#include <math.h>
#include <cairo.h>

#define NEMOPATH_CUBIC_BEZIER_FLATTEN_STEPS		(100)
#define NEMOPATH_CUBIC_BEZIER_FLATTEN_GAPS			(3)

#define NEMOPATH_ARC_MAGIC		((double)0.5522847498f)

struct nemopath {
	cairo_path_data_t *pathdata;
	int npathdata, spathdata;
	int lastindex;
	cairo_path_data_t lastdata;

	double *pathdist;
	int npathdist, spathdist;

	cairo_path_t *cpath;
	int dirty;

	int index;
	double offset;
	cairo_path_data_t cp;
	cairo_path_data_t lp;

	double extents[4];
	double length;

	double cx[4], cy[4];
};

extern struct nemopath *nemopath_create(void);
extern void nemopath_destroy(struct nemopath *path);

extern void nemopath_move_to(struct nemopath *path, double x, double y);
extern void nemopath_line_to(struct nemopath *path, double x, double y);
extern void nemopath_curve_to(struct nemopath *path, double x1, double y1, double x2, double y2, double x3, double y3);
extern void nemopath_quadratic_curve_to(struct nemopath *path, double x1, double y1, double x2, double y2);
extern void nemopath_arc_to(struct nemopath *path, double cx, double cy, double rx, double ry);
extern void nemopath_close_path(struct nemopath *path);
extern void nemopath_curve_move_to(struct nemopath *path, double x, double y);
extern void nemopath_curve_line_to(struct nemopath *path, double x, double y);
extern int nemopath_curve_twist_to(struct nemopath *path, double x, double y, double startradius, double spaceperloop, double starttheta, double endtheta, double thetastep);
extern void nemopath_circle(struct nemopath *path, double cx, double cy, double r);
extern void nemopath_rect(struct nemopath *path, double x, double y, double w, double h);

extern int nemopath_append_path(struct nemopath *path, cairo_path_t *cpath);
extern int nemopath_append_cmd(struct nemopath *path, const char *cmd);
extern void nemopath_clear_path(struct nemopath *path);

extern cairo_path_t *nemopath_get_cairo_path(struct nemopath *path);

extern int nemopath_draw_all(struct nemopath *path, cairo_t *cr, double *extents);
extern int nemopath_draw_subpath(struct nemopath *path, cairo_t *cr, double *extents);
extern int nemopath_extents_subpath(struct nemopath *path, double *extents);

extern void nemopath_translate(struct nemopath *path, double x, double y);
extern void nemopath_scale(struct nemopath *path, double sx, double sy);
extern void nemopath_transform(struct nemopath *path, cairo_matrix_t *matrix);

extern double nemopath_get_position(struct nemopath *path, double offset, double *px, double *py);
extern double nemopath_get_progress(struct nemopath *path, double start, double end, double steps, double x, double y);

extern void nemopath_extents(struct nemopath *path, double *extents);
extern int nemopath_elements(struct nemopath *path);
extern double nemopath_width(struct nemopath *path);
extern double nemopath_height(struct nemopath *path);
extern double nemopath_length(struct nemopath *path);
extern void nemopath_reset(struct nemopath *path);

extern struct nemopath *nemopath_clone(struct nemopath *path);
extern int nemopath_flatten(struct nemopath *path);
extern int nemopath_split(struct nemopath *path, int elements);
extern int nemopath_tween(struct nemopath *path, struct nemopath *spath, struct nemopath *dpath, double progress);

extern void nemopath_dump(struct nemopath *path, FILE *out);

#endif
