#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <toyzmatrix.hpp>
#include <nemomisc.h>

struct toyzmatrix *nemotoyz_matrix_create(void)
{
	struct toyzmatrix *matrix;

	matrix = new toyzmatrix;
	matrix->matrix = new SkMatrix;

	return matrix;
}

void nemotoyz_matrix_destroy(struct toyzmatrix *matrix)
{
	delete matrix->matrix;
	delete matrix;
}

void nemotoyz_matrix_set(struct toyzmatrix *matrix, float *ms)
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

void nemotoyz_matrix_identity(struct toyzmatrix *matrix)
{
	matrix->matrix->setIdentity();
}

void nemotoyz_matrix_translate(struct toyzmatrix *matrix, float tx, float ty)
{
	matrix->matrix->setTranslate(tx, ty);
}

void nemotoyz_matrix_scale(struct toyzmatrix *matrix, float sx, float sy)
{
	matrix->matrix->setScale(sx, sy);
}

void nemotoyz_matrix_rotate(struct toyzmatrix *matrix, float rz)
{
	matrix->matrix->setRotate(rz);
}

void nemotoyz_matrix_post_translate(struct toyzmatrix *matrix, float tx, float ty)
{
	matrix->matrix->postTranslate(tx, ty);
}

void nemotoyz_matrix_post_scale(struct toyzmatrix *matrix, float sx, float sy)
{
	matrix->matrix->postScale(sx, sy);
}

void nemotoyz_matrix_post_rotate(struct toyzmatrix *matrix, float rz)
{
	matrix->matrix->postRotate(rz);
}

void nemotoyz_matrix_concat(struct toyzmatrix *matrix, struct toyzmatrix *smatrix)
{
	matrix->matrix->postConcat(*smatrix->matrix);
}

void nemotoyz_matrix_map_point(struct toyzmatrix *matrix, float *x, float *y)
{
	SkPoint p = matrix->matrix->mapXY(*x, *y);

	*x = p.x();
	*y = p.y();
}

void nemotoyz_matrix_map_vector(struct toyzmatrix *matrix, float *x, float *y)
{
	SkVector v;
	v.set(*x, *y);

	matrix->matrix->mapVectors(&v, 1);

	*x = v.x();
	*y = v.y();
}

void nemotoyz_matrix_map_rectangle(struct toyzmatrix *matrix, float *x, float *y, float *w, float *h)
{
	SkRect rect = SkRect::MakeXYWH(*x, *y, *w, *h);

	matrix->matrix->mapRect(&rect);

	*x = rect.x();
	*y = rect.y();
	*w = rect.width();
	*h = rect.height();
}
