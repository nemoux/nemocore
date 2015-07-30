#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showmatrix.h>
#include <showmatrix.hpp>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_matrix_create(int type)
{
	struct showmatrix *matrix;
	struct showone *one;

	matrix = (struct showmatrix *)malloc(sizeof(struct showmatrix));
	if (matrix == NULL)
		return NULL;
	memset(matrix, 0, sizeof(struct showmatrix));

	matrix->cc = new showmatrix_t;

	one = &matrix->base;
	one->type = NEMOSHOW_MATRIX_TYPE;
	one->sub = type;
	one->destroy = nemoshow_matrix_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &matrix->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &matrix->y, sizeof(double));

	matrix->matrices = (struct showone **)malloc(sizeof(struct showone *) * 4);
	matrix->nmatrices = 0;
	matrix->smatrices = 4;

	return one;
}

void nemoshow_matrix_destroy(struct showone *one)
{
	struct showmatrix *matrix = NEMOSHOW_MATRIX(one);

	nemoshow_one_finish(one);

	delete static_cast<showmatrix_t *>(matrix->cc);

	free(matrix->matrices);
	free(matrix);
}

int nemoshow_matrix_arrange(struct nemoshow *show, struct showone *one)
{
	struct showmatrix *matrix = NEMOSHOW_MATRIX(one);

	NEMOSHOW_MATRIX_CC(matrix, matrix) = new SkMatrix;

	return 0;
}

int nemoshow_matrix_update(struct nemoshow *show, struct showone *one)
{
	struct showmatrix *matrix = NEMOSHOW_MATRIX(one);

	if (one->sub == NEMOSHOW_SCALE_MATRIX) {
		NEMOSHOW_MATRIX_CC(matrix, matrix)->setScale(matrix->x, matrix->y);
	} else if (one->sub == NEMOSHOW_ROTATE_MATRIX) {
		NEMOSHOW_MATRIX_CC(matrix, matrix)->setRotate(matrix->x);
	} else if (one->sub == NEMOSHOW_TRANSLATE_MATRIX) {
		NEMOSHOW_MATRIX_CC(matrix, matrix)->setTranslate(matrix->x, matrix->y);
	} else if (one->sub == NEMOSHOW_MATRIX_MATRIX) {
		struct showone *child;
		int i;

		NEMOSHOW_MATRIX_CC(matrix, matrix)->setIdentity();

		for (i = 0; i < matrix->nmatrices; i++) {
			child = matrix->matrices[i];

			nemoshow_matrix_update(show, child);

			NEMOSHOW_MATRIX_CC(matrix, matrix)->postConcat(
					*NEMOSHOW_MATRIX_CC(
						NEMOSHOW_MATRIX(child),
						matrix));
		}
	}

	return 0;
}
