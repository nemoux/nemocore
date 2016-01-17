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
	NEMOSHOW_LAST_FILTER
} NemoShowFilterType;

struct showfilter {
	struct showone base;

	double r;

	double dx, dy, dz;
	double ambient;
	double specular;

	void *cc;
};

#define NEMOSHOW_FILTER(one)					((struct showfilter *)container_of(one, struct showfilter, base))
#define	NEMOSHOW_FILTER_AT(one, at)		(NEMOSHOW_FILTER(one)->at)

extern struct showone *nemoshow_filter_create(int type);
extern void nemoshow_filter_destroy(struct showone *one);

extern int nemoshow_filter_arrange(struct showone *one);
extern int nemoshow_filter_update(struct showone *one);

extern void nemoshow_filter_set_blur(struct showone *one, const char *flags, const char *style, double r);
extern void nemoshow_filter_set_light(struct showone *one, double dx, double dy, double dz, double ambient, double specular);

extern void nemoshow_filter_set_radius(struct showone *one, double r);
extern void nemoshow_filter_set_direction(struct showone *one, double dx, double dy, double dz);
extern void nemoshow_filter_set_ambient(struct showone *one, double ambient);
extern void nemoshow_filter_set_specular(struct showone *one, double specular);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
