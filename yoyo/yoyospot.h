#ifndef __NEMOYOYO_SPOT_H__
#define __NEMOYOYO_SPOT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemocook.h>
#include <nemolist.h>

struct yoyospot {
	struct cookpoly *poly;
	struct cooktrans *trans;

	struct {
		float tx, ty;
		float sx, sy;
		float rz;

		float w, h;
	} geometry;

	struct nemolist link;
};

extern struct yoyospot *nemoyoyo_spot_create(void);
extern void nemoyoyo_spot_destroy(struct yoyospot *spot);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
