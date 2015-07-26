#ifndef	__NEMOSHOW_SCENE_H__
#define	__NEMOSHOW_SCENE_H__

#include <stdint.h>

#include <showone.h>

struct showscene {
	struct showone base;

	double width, height;
};

#define	NEMOSHOW_SCENE(one)		((struct showscene *)container_of(one, struct showscene, base))

extern struct showone *nemoshow_scene_create(void);
extern void nemoshow_scene_destroy(struct showone *one);

extern void nemoshow_scene_dump(struct showone *one, FILE *out);

#endif
