#ifndef __NEMOSHOW_BLUR_H__
#define __NEMOSHOW_BLUR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showblur {
	struct showone base;

	double r;

	void *cc;
};

#define NEMOSHOW_BLUR(one)					((struct showblur *)container_of(one, struct showblur, base))
#define	NEMOSHOW_BLUR_AT(one, at)		(NEMOSHOW_BLUR(one)->at)

extern struct showone *nemoshow_blur_create(void);
extern void nemoshow_blur_destroy(struct showone *one);

extern int nemoshow_blur_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_blur_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_blur_set_filter(struct showone *one, const char *flags, const char *style, double r);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
