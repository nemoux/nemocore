#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotozz.h>
#include <tozzmatrix.hpp>
#include <nemomisc.h>

struct tozzmatrix *nemotozz_matrix_create(void)
{
	struct tozzmatrix *matrix;

	matrix = new tozzmatrix;
	matrix->matrix = new SkMatrix;

	return matrix;
}

void nemotozz_matrix_destroy(struct tozzmatrix *matrix)
{
	delete matrix->matrix;
	delete matrix;
}

void nemotozz_matrix_set(struct tozzmatrix *matrix, float *ms)
{
	SkScalar args[9] = {
		SkDoubleToScalar(ms[0]),
		SkDoubleToScalar(ms[1]),
		SkDoubleToScalar(ms[2]),
		SkDoubleToScalar(ms[3]),
		SkDoubleToScalar(ms[4]),
		SkDoubleToScalar(ms[5]),
		SkDoubleToScalar(ms[6]),
		SkDoubleToScalar(ms[7]),
		SkDoubleToScalar(ms[8])
	};

	matrix->matrix->set9(args);
}

void nemotozz_matrix_identity(struct tozzmatrix *matrix)
{
	matrix->matrix->setIdentity();
}

void nemotozz_matrix_translate(struct tozzmatrix *matrix, float tx, float ty)
{
	matrix->matrix->setTranslate(tx, ty);
}

void nemotozz_matrix_scale(struct tozzmatrix *matrix, float sx, float sy)
{
	matrix->matrix->setScale(sx, sy);
}

void nemotozz_matrix_rotate(struct tozzmatrix *matrix, float rz)
{
	matrix->matrix->setRotate(rz);
}

void nemotozz_matrix_post_translate(struct tozzmatrix *matrix, float tx, float ty)
{
	matrix->matrix->postTranslate(tx, ty);
}

void nemotozz_matrix_post_scale(struct tozzmatrix *matrix, float sx, float sy)
{
	matrix->matrix->postScale(sx, sy);
}

void nemotozz_matrix_post_rotate(struct tozzmatrix *matrix, float rz)
{
	matrix->matrix->postRotate(rz);
}

void nemotozz_matrix_concat(struct tozzmatrix *matrix, struct tozzmatrix *smatrix)
{
	matrix->matrix->postConcat(*smatrix->matrix);
}

void nemotozz_matrix_invert(struct tozzmatrix *matrix, struct tozzmatrix *smatrix)
{
	smatrix->matrix->invert(matrix->matrix);
}

void nemotozz_matrix_map_point(struct tozzmatrix *matrix, float *x, float *y)
{
	SkPoint p = matrix->matrix->mapXY(*x, *y);

	*x = p.x();
	*y = p.y();
}

void nemotozz_matrix_map_vector(struct tozzmatrix *matrix, float *x, float *y)
{
	SkVector v;
	v.set(*x, *y);

	matrix->matrix->mapVectors(&v, 1);

	*x = v.x();
	*y = v.y();
}

void nemotozz_matrix_map_rectangle(struct tozzmatrix *matrix, float *x, float *y, float *w, float *h)
{
	SkRect rect = SkRect::MakeXYWH(*x, *y, *w, *h);

	matrix->matrix->mapRect(&rect);

	*x = rect.x();
	*y = rect.y();
	*w = rect.width();
	*h = rect.height();
}
