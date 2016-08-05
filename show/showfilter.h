#ifndef __NEMOSHOW_FILTER_H__
#define __NEMOSHOW_FILTER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_FILTER = 0,
	NEMOSHOW_BLUR_FILTER = 1,
	NEMOSHOW_EMBOSS_FILTER = 2,
	NEMOSHOW_SHADOW_FILTER = 3,
	NEMOSHOW_LAST_FILTER
} NemoShowFilterType;

typedef enum {
	NEMOSHOW_FILTER_MASK_TYPE = 0,
	NEMOSHOW_FILTER_IMAGE_TYPE = 1,
	NEMOSHOW_FILTER_COLOR_TYPE = 2,
	NEMOSHOW_FILTER_LAST_TYPE
} NemoShowFilterBaseType;

typedef enum {
	NEMOSHOW_FILTER_SHADOW_AND_FOREGROUND_MODE = 0,
	NEMOSHOW_FILTER_SHADOW_ONLY_MODE = 1,
	NEMOSHOW_FILTER_SHADOW_LAST_MODE
} NemoShowFilterShadowMode;

struct showfilter {
	struct showone base;

	int type;

	double r;

	double dx, dy, dz;
	double sx, sy;
	double ambient;
	double specular;

	double fills[4];
	int mode;

	void *cc;
};

#define NEMOSHOW_FILTER(one)					((struct showfilter *)container_of(one, struct showfilter, base))
#define	NEMOSHOW_FILTER_AT(one, at)		(NEMOSHOW_FILTER(one)->at)

extern struct showone *nemoshow_filter_create(int type);
extern void nemoshow_filter_destroy(struct showone *one);

extern int nemoshow_filter_update(struct showone *one);

extern void nemoshow_filter_set_blur(struct showone *one, const char *style, double r);
extern void nemoshow_filter_set_light(struct showone *one, double dx, double dy, double dz, double ambient, double specular);

extern void nemoshow_filter_set_radius(struct showone *one, double r);
extern void nemoshow_filter_set_direction(struct showone *one, double dx, double dy, double dz);
extern void nemoshow_filter_set_ambient(struct showone *one, double ambient);
extern void nemoshow_filter_set_specular(struct showone *one, double specular);

static inline void nemoshow_filter_set_color(struct showone *one, double r, double g, double b, double a)
{
	struct showfilter *filter = NEMOSHOW_FILTER(one);

	filter->fills[NEMOSHOW_RED_COLOR] = r;
	filter->fills[NEMOSHOW_GREEN_COLOR] = g;
	filter->fills[NEMOSHOW_BLUE_COLOR] = b;
	filter->fills[NEMOSHOW_ALPHA_COLOR] = a;

	nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_filter_set_dx(struct showone *one, double dx)
{
	NEMOSHOW_FILTER_AT(one, dx) = dx;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

static inline void nemoshow_filter_set_dy(struct showone *one, double dy)
{
	NEMOSHOW_FILTER_AT(one, dy) = dy;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

static inline void nemoshow_filter_set_sx(struct showone *one, double sx)
{
	NEMOSHOW_FILTER_AT(one, sx) = sx;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

static inline void nemoshow_filter_set_sy(struct showone *one, double sy)
{
	NEMOSHOW_FILTER_AT(one, sy) = sy;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

static inline void nemoshow_filter_set_mode(struct showone *one, int mode)
{
	NEMOSHOW_FILTER_AT(one, mode) = mode;

	nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
}

typedef enum {
	NEMOBLUR_INNER_SMALL_TYPE = 0,
	NEMOBLUR_INNER_MEDIUM_TYPE = 1,
	NEMOBLUR_INNER_LARGE_TYPE = 2,
	NEMOBLUR_OUTER_SMALL_TYPE = 3,
	NEMOBLUR_OUTER_MEDIUM_TYPE = 4,
	NEMOBLUR_OUTER_LARGE_TYPE = 5,
	NEMOBLUR_SOLID_SMALL_TYPE = 6,
	NEMOBLUR_SOLID_MEDIUM_TYPE = 7,
	NEMOBLUR_SOLID_LARGE_TYPE = 8,
	NEMOBLUR_LAST_TYPE
} NemoBlurType;

extern struct showone *nemoblurs[NEMOBLUR_LAST_TYPE];

#define NEMOSHOW_INNER_SMALL_BLUR						(nemoblurs[NEMOBLUR_INNER_SMALL_TYPE])
#define NEMOSHOW_INNER_MEDIUM_BLUR					(nemoblurs[NEMOBLUR_INNER_SMALL_TYPE])
#define NEMOSHOW_INNER_LARGE_BLUR						(nemoblurs[NEMOBLUR_INNER_SMALL_TYPE])
#define NEMOSHOW_OUTER_SMALL_BLUR						(nemoblurs[NEMOBLUR_OUTER_SMALL_TYPE])
#define NEMOSHOW_OUTER_MEDIUM_BLUR					(nemoblurs[NEMOBLUR_OUTER_SMALL_TYPE])
#define NEMOSHOW_OUTER_LARGE_BLUR						(nemoblurs[NEMOBLUR_OUTER_SMALL_TYPE])
#define NEMOSHOW_SOLID_SMALL_BLUR						(nemoblurs[NEMOBLUR_SOLID_SMALL_TYPE])
#define NEMOSHOW_SOLID_MEDIUM_BLUR					(nemoblurs[NEMOBLUR_SOLID_SMALL_TYPE])
#define NEMOSHOW_SOLID_LARGE_BLUR						(nemoblurs[NEMOBLUR_SOLID_SMALL_TYPE])

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
