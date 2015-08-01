#ifndef	__NEMOSHOW_SCENE_H__
#define	__NEMOSHOW_SCENE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showscene {
	struct showone base;

	double width, height;
};

#define NEMOSHOW_SCENE(one)					((struct showscene *)container_of(one, struct showscene, base))
#define NEMOSHOW_SCENE_AT(one, at)	(NEMOSHOW_SCENE(one)->at)

extern struct showone *nemoshow_scene_create(void);
extern void nemoshow_scene_destroy(struct showone *one);

extern int nemoshow_scene_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
