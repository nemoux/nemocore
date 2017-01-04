#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookcamera.h>
#include <nemomisc.h>

struct cookcamera *nemocook_camera_create(void)
{
	struct cookcamera *camera;

	camera = (struct cookcamera *)malloc(sizeof(struct cookcamera));
	if (camera == NULL)
		return NULL;
	memset(camera, 0, sizeof(struct cookcamera));

	nemomatrix_init_identity(&camera->matrix);

	return camera;
}

void nemocook_camera_destroy(struct cookcamera *camera)
{
	free(camera);
}

void nemocook_camera_set_type(struct cookcamera *camera, int type)
{
	camera->type = type;
}

float *nemocook_camera_get_array(struct cookcamera *camera)
{
	return nemomatrix_get_array(&camera->matrix);
}

struct nemomatrix *nemocook_camera_get_matrix(struct cookcamera *camera)
{
	return &camera->matrix;
}

void nemocook_camera_update(struct cookcamera *camera)
{
	nemomatrix_init_identity(&camera->matrix);

	if (camera->type == NEMOCOOK_CAMERA_ORTHOGONAL_TYPE) {
		nemomatrix_orthogonal(&camera->matrix,
				camera->u.orthogonal.left,
				camera->u.orthogonal.right,
				camera->u.orthogonal.bottom,
				camera->u.orthogonal.top,
				camera->u.orthogonal.near,
				camera->u.orthogonal.far);
	} else if (camera->type == NEMOCOOK_CAMERA_PERSPECTIVE_TYPE) {
		nemomatrix_perspective(&camera->matrix,
				camera->u.perspective.left,
				camera->u.perspective.right,
				camera->u.perspective.bottom,
				camera->u.perspective.top,
				camera->u.perspective.near,
				camera->u.perspective.far);
	} else if (camera->type == NEMOCOOK_CAMERA_ASYMMETRIC_TYPE) {
		nemomatrix_asymmetric(&camera->matrix,
				camera->u.asymmetric.a,
				camera->u.asymmetric.b,
				camera->u.asymmetric.c,
				camera->u.asymmetric.e,
				camera->u.asymmetric.near,
				camera->u.asymmetric.far);
	}
}
