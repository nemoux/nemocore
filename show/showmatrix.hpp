#ifndef __NEMOSHOW_MATRIX_HPP__
#define __NEMOSHOW_MATRIX_HPP__

#include <skiaconfig.hpp>

typedef struct _showmatrix {
	SkMatrix *matrix;
} showmatrix_t;

#define NEMOSHOW_MATRIX_CC(base, name)				(((showmatrix_t *)((base)->cc))->name)

#endif
