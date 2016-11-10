#ifndef __NEMO_TRANS_H__
#define __NEMO_TRANS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoease.h>
#include <nemoattr.h>
#include <nemolist.h>

struct transone {
	struct nemolist link;

	struct nemoattr attr;
	int is_double;

	double sattr;
	double eattr;
};

struct nemotrans {
	struct nemolist link;

	struct nemoease ease;

	struct nemolist list;

	uint32_t duration;
	uint32_t delay;

	uint32_t stime;
	uint32_t etime;
};

extern struct nemotrans *nemotrans_create(int type, uint32_t duration, uint32_t delay);
extern void nemotrans_destroy(struct nemotrans *trans);

extern void nemotrans_ease_set_type(struct nemotrans *trans, int type);
extern void nemotrans_ease_set_bezier(struct nemotrans *trans, double x0, double y0, double x1, double y1);

extern int nemotrans_dispatch(struct nemotrans *trans, uint32_t time);

extern void nemotrans_set_float(struct nemotrans *trans, float *var, float value);
extern void nemotrans_set_double(struct nemotrans *trans, double *var, double value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
