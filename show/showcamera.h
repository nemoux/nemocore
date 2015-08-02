#ifndef __NEMOSHOW_CAMERA_H__
#define __NEMOSHOW_CAMERA_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotale.h>
#include <talegl.h>

#include <showone.h>

struct showcamera {
	struct showone base;

	double sx, sy;

	void *cc;
};

#define NEMOSHOW_CAMERA(one)					((struct showcamera *)container_of(one, struct showcamera, base))
#define	NEMOSHOW_CAMERA_AT(one, at)		(NEMOSHOW_CAMERA(one)->at)

extern struct showone *nemoshow_camera_create(void);
extern void nemoshow_camera_destroy(struct showone *one);

extern int nemoshow_camera_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_camera_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
