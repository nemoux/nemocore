#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showcamera.h>
#include <showcamera.hpp>
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

	camera->cc = new showcamera_t;

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

	delete static_cast<showcamera_t *>(camera->cc);

	free(camera);
}

int nemoshow_camera_arrange(struct nemoshow *show, struct showone *one)
{
	struct showcamera *camera = NEMOSHOW_CAMERA(one);

	NEMOSHOW_CAMERA_CC(camera, matrix) = new SkMatrix;

	return 0;
}

int nemoshow_camera_update(struct nemoshow *show, struct showone *one)
{
	struct showcamera *camera = NEMOSHOW_CAMERA(one);
	struct showone *child;
	int needs_scale = 0;
	int i;

	NEMOSHOW_CAMERA_CC(camera, matrix)->setIdentity();

	camera->sx = 1.0f;
	camera->sy = 1.0f;

	for (i = 0; i < one->nchildren; i++) {
		child = one->children[i];

		if (child->type != NEMOSHOW_MATRIX_TYPE)
			continue;

		if (child->sub == NEMOSHOW_SCALE_MATRIX) {
			NEMOSHOW_CAMERA_CC(camera, matrix)->postScale(
					NEMOSHOW_MATRIX_AT(child, x),
					NEMOSHOW_MATRIX_AT(child, y));

			camera->sx = camera->sx * NEMOSHOW_MATRIX_AT(child, x);
			camera->sy = camera->sy * NEMOSHOW_MATRIX_AT(child, y);

			if (child->state == NEMOSHOW_NORMAL_STATE)
				needs_scale = 1;
		} else if (child->sub == NEMOSHOW_ROTATE_MATRIX) {
			NEMOSHOW_CAMERA_CC(camera, matrix)->postRotate(
					NEMOSHOW_MATRIX_AT(child, x));
		} else if (child->sub == NEMOSHOW_TRANSLATE_MATRIX) {
			NEMOSHOW_CAMERA_CC(camera, matrix)->postTranslate(
					NEMOSHOW_MATRIX_AT(child, x),
					NEMOSHOW_MATRIX_AT(child, y));
		}
	}

	if (show->camera == one) {
		float d[9];

		NEMOSHOW_CAMERA_CC(camera, matrix)->get9(d);

		nemotale_transform(show->tale, d);

		if (needs_scale == 0 || nemoshow_set_scale(show, camera->sx, camera->sy) == 0)
			nemoshow_flush_canvas_all(show);
	}

	return 0;
}
