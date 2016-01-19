#ifndef	__NEMOSHOW_HELPER_HPP__
#define	__NEMOSHOW_HELPER_HPP__

extern double nemoshow_helper_get_path_length(SkPath *path);
extern void nemoshow_helper_draw_path(SkPath &dst, SkPath *src, double length, double from, double to);
extern void nemoshow_helper_evaluate_path(SkPath *src, double length, double t, double *x, double *y, double *r);

#endif
