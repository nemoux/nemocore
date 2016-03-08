#ifndef	__NEMOSHOW_RING_H__
#define	__NEMOSHOW_RING_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoshow.h>

struct showring {
	struct showone *icon0;
	struct showone *icon1;
};

extern struct showone *nemoshow_ring_create(int32_t width, int32_t height);
extern void nemoshow_ring_destroy(struct showone *one);

extern int nemoshow_ring_set_dattr(struct showone *one, const char *attr, double value);
extern int nemoshow_ring_set_sattr(struct showone *one, const char *attr, const char *value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
