#ifndef __NEMO_TRIX_H__
#define __NEMO_TRIX_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

struct nemotrix {
	struct nemolist list;
};

extern struct nemotrix *nemotrix_create(void);
extern void nemotrix_destroy(struct nemotrix *trix);

extern int nemotrix_triangulate(struct nemotrix *trix, float *vertices, int nvertices);

extern int nemotrix_get_triangles(struct nemotrix *trix, float *triangles);
extern int nemotrix_get_edges(struct nemotrix *trix, float *edges);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
