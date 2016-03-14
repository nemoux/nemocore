#ifndef __NEMOSHOW_MATRIX_HPP__
#define __NEMOSHOW_MATRIX_HPP__

typedef struct _showmatrix {
	SkMatrix *matrix;
} showmatrix_t;

#define NEMOSHOW_MATRIX_CC(base, name)				(((showmatrix_t *)((base)->cc))->name)
#define NEMOSHOW_MATRIX_ATCC(one, name)				(NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(one), name))

#endif
