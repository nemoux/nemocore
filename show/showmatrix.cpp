#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

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
	NEMOSHOW_MATRIX_CC(matrix, matrix) = new SkMatrix;

	one = &matrix->base;
	one->type = NEMOSHOW_MATRIX_TYPE;
	one->sub = type;
	one->update = nemoshow_matrix_update;
	one->destroy = nemoshow_matrix_destroy;
	one->attach = nemoshow_matrix_attach_one;
	one->detach = nemoshow_matrix_detach_one;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &matrix->x, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &matrix->y, sizeof(double));

	return one;
}

void nemoshow_matrix_destroy(struct showone *one)
{
	struct showmatrix *matrix = NEMOSHOW_MATRIX(one);
	struct showone *child, *nchild;

	nemolist_for_each_safe(child, nchild, &one->children_list, children_link)
		nemoshow_one_destroy_all(child);

	nemoshow_one_finish(one);

	if (NEMOSHOW_MATRIX_CC(matrix, matrix) != NULL)
		delete NEMOSHOW_MATRIX_CC(matrix, matrix);

	delete static_cast<showmatrix_t *>(matrix->cc);

	free(matrix);
}

void nemoshow_matrix_attach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_attach_one(parent, one);

	nemoshow_one_reference_one(parent, one, NEMOSHOW_MATRIX_DIRTY, -1);
}

void nemoshow_matrix_detach_one(struct showone *parent, struct showone *one)
{
	nemoshow_one_detach_one(parent, one);
}

int nemoshow_matrix_arrange(struct showone *one)
{
	struct showmatrix *matrix = NEMOSHOW_MATRIX(one);

	return 0;
}

int nemoshow_matrix_update(struct showone *one)
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

		nemolist_for_each(child, &one->children_list, children_link) {
			nemoshow_matrix_update(child);

			NEMOSHOW_MATRIX_CC(matrix, matrix)->postConcat(
					*NEMOSHOW_MATRIX_CC(
						NEMOSHOW_MATRIX(child),
						matrix));
		}
	}

	return 0;
}
