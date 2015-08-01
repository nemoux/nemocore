#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showcamera.h>
#include <showmatrix.h>
#include <showmatrix.hpp>
#include <nemoshow.h>
#include <nemobox.h>
#include <nemomisc.h>

struct showone *nemoshow_camera_create(void)
{
	struct showcamera *camera;
	struct showone *one;

	camera = (struct showcamera *)malloc(sizeof(struct showcamera));
	if (camera == NULL)
		return NULL;
	memset(camera, 0, sizeof(struct showcamera));

	one = &camera->base;
	one->type = NEMOSHOW_CAMERA_TYPE;
	one->update = nemoshow_camera_update;
	one->destroy = nemoshow_camera_destroy;

	nemoshow_one_prepare(one);

	return one;
}

void nemoshow_camera_destroy(struct showone *one)
{
	struct showcamera *camera = NEMOSHOW_CAMERA(one);

	nemoshow_one_finish(one);

	free(camera);
}

int nemoshow_camera_arrange(struct nemoshow *show, struct showone *one)
{
	struct showcamera *camera = NEMOSHOW_CAMERA(one);
	struct showone *matrix;

	matrix = nemoshow_search_one(show, nemoobject_gets(&one->object, "matrix"));
	if (matrix != NULL) {
		camera->matrix = matrix;

		NEMOBOX_APPEND(matrix->refs, matrix->srefs, matrix->nrefs, one);
	}

	return 0;
}

int nemoshow_camera_update(struct nemoshow *show, struct showone *one)
{
	if (show->camera == one) {
		struct showcamera *camera = NEMOSHOW_CAMERA(one);
		float d[9];

		if (camera->matrix->dirty != 0) {
			nemoshow_matrix_update(show, camera->matrix);

			camera->matrix->dirty = 0;
		}

		NEMOSHOW_MATRIX_CC(NEMOSHOW_MATRIX(camera->matrix), matrix)->get9(d);

		nemotale_transform_gl(show->tale, d);

		nemoshow_dirty_scene(show);
	}

	return 0;
}
