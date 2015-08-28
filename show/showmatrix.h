#ifndef __NEMOSHOW_MATRIX_H__
#define __NEMOSHOW_MATRIX_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_MATRIX = 0,
	NEMOSHOW_SCALE_MATRIX = 1,
	NEMOSHOW_ROTATE_MATRIX = 2,
	NEMOSHOW_TRANSLATE_MATRIX = 3,
	NEMOSHOW_MATRIX_MATRIX = 4,
	NEMOSHOW_LAST_MATRIX
} NemoShowMatrixType;

struct showmatrix {
	struct showone base;

	double x, y;

	void *cc;
};

#define NEMOSHOW_MATRIX(one)					((struct showmatrix *)container_of(one, struct showmatrix, base))
#define	NEMOSHOW_MATRIX_AT(one, at)		(NEMOSHOW_MATRIX(one)->at)

extern struct showone *nemoshow_matrix_create(int type);
extern void nemoshow_matrix_destroy(struct showone *one);

extern int nemoshow_matrix_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_matrix_update(struct nemoshow *show, struct showone *one);

static inline void nemoshow_matrix_set_x(struct showone *one, double x)
{
	NEMOSHOW_MATRIX_AT(one, x) = x;
}

static inline void nemoshow_matrix_set_y(struct showone *one, double y)
{
	NEMOSHOW_MATRIX_AT(one, y) = y;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
