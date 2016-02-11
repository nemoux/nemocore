#ifndef __NEMO_SPIN_H__
#define __NEMO_SPIN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <math.h>

#include <nemomatrix.h>

struct nemospin {
	struct nemovector vec0, vec1;
	struct nemoquaternion quat0, quat1;

	int32_t width, height;
};

extern void nemospin_init(struct nemospin *spin, int32_t width, int32_t height);
extern void nemospin_resize(struct nemospin *spin, int32_t width, int32_t height);
extern void nemospin_reset(struct nemospin *spin, float x, float y);
extern void nemospin_update(struct nemospin *spin, float x, float y);

static struct nemoquaternion *nemospin_get_quaternion(struct nemospin *spin)
{
	return &spin->quat1;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
