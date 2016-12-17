#ifndef __NEMOCOOK_CAMERA_H__
#define __NEMOCOOK_CAMERA_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <nemomatrix.h>

typedef enum {
	NEMOCOOK_CAMERA_NONE_TYPE = 0,
	NEMOCOOK_CAMERA_ORTHOGONAL_TYPE = 1,
	NEMOCOOK_CAMERA_PERSPECTIVE_TYPE = 2,
	NEMOCOOK_CAMERA_ASYMMETRIC_TYPE = 3,
	NEMOCOOK_CAMERA_LAST_TYPE
} NemoCookCameraType;

struct cookcamera {
	struct nemomatrix matrix;

	int type;

	union {
		struct {
			float left, right;
			float bottom, top;
			float near, far;
		} orthogonal;

		struct {
			float left, right;
			float bottom, top;
			float near, far;
		} perspective;

		struct {
			float a[3], b[3], c[3], e[3];
			float near, far;
		} asymmetric;
	} u;
};

extern struct cookcamera *nemocook_camera_create(void);
extern void nemocook_camera_destroy(struct cookcamera *camera);

extern void nemocook_camera_set_type(struct cookcamera *camera, int type);

extern float *nemocook_camera_get_array(struct cookcamera *camera);
extern struct nemomatrix *nemocook_camera_get_matrix(struct cookcamera *camera);

extern void nemocook_camera_update(struct cookcamera *camera);

static inline void nemocook_camera_set_orthogonal_left(struct cookcamera *camera, float left)
{
	camera->u.orthogonal.left = left;
}

static inline void nemocook_camera_set_orthogonal_right(struct cookcamera *camera, float right)
{
	camera->u.orthogonal.right = right;
}

static inline void nemocook_camera_set_orthogonal_bottom(struct cookcamera *camera, float bottom)
{
	camera->u.orthogonal.bottom = bottom;
}

static inline void nemocook_camera_set_orthogonal_top(struct cookcamera *camera, float top)
{
	camera->u.orthogonal.top = top;
}

static inline void nemocook_camera_set_orthogonal_near(struct cookcamera *camera, float near)
{
	camera->u.orthogonal.near = near;
}

static inline void nemocook_camera_set_orthogonal_far(struct cookcamera *camera, float far)
{
	camera->u.orthogonal.far = far;
}

static inline void nemocook_camera_set_perspective_left(struct cookcamera *camera, float left)
{
	camera->u.perspective.left = left;
}

static inline void nemocook_camera_set_perspective_right(struct cookcamera *camera, float right)
{
	camera->u.perspective.right = right;
}

static inline void nemocook_camera_set_perspective_bottom(struct cookcamera *camera, float bottom)
{
	camera->u.perspective.bottom = bottom;
}

static inline void nemocook_camera_set_perspective_top(struct cookcamera *camera, float top)
{
	camera->u.perspective.top = top;
}

static inline void nemocook_camera_set_perspective_near(struct cookcamera *camera, float near)
{
	camera->u.perspective.near = near;
}

static inline void nemocook_camera_set_perspective_far(struct cookcamera *camera, float far)
{
	camera->u.perspective.far = far;
}

static inline void nemocook_camera_set_asymmetric_ax(struct cookcamera *camera, float x)
{
	camera->u.asymmetric.a[0] = x;
}

static inline void nemocook_camera_set_asymmetric_ay(struct cookcamera *camera, float y)
{
	camera->u.asymmetric.a[1] = y;
}

static inline void nemocook_camera_set_asymmetric_az(struct cookcamera *camera, float z)
{
	camera->u.asymmetric.a[2] = z;
}

static inline void nemocook_camera_set_asymmetric_bx(struct cookcamera *camera, float x)
{
	camera->u.asymmetric.b[0] = x;
}

static inline void nemocook_camera_set_asymmetric_by(struct cookcamera *camera, float y)
{
	camera->u.asymmetric.b[1] = y;
}

static inline void nemocook_camera_set_asymmetric_bz(struct cookcamera *camera, float z)
{
	camera->u.asymmetric.b[2] = z;
}

static inline void nemocook_camera_set_asymmetric_cx(struct cookcamera *camera, float x)
{
	camera->u.asymmetric.c[0] = x;
}

static inline void nemocook_camera_set_asymmetric_cy(struct cookcamera *camera, float y)
{
	camera->u.asymmetric.c[1] = y;
}

static inline void nemocook_camera_set_asymmetric_cz(struct cookcamera *camera, float z)
{
	camera->u.asymmetric.c[2] = z;
}

static inline void nemocook_camera_set_asymmetric_ex(struct cookcamera *camera, float x)
{
	camera->u.asymmetric.e[0] = x;
}

static inline void nemocook_camera_set_asymmetric_ey(struct cookcamera *camera, float y)
{
	camera->u.asymmetric.e[1] = y;
}

static inline void nemocook_camera_set_asymmetric_ez(struct cookcamera *camera, float z)
{
	camera->u.asymmetric.e[2] = z;
}

static inline void nemocook_camera_set_asymmetric_near(struct cookcamera *camera, float near)
{
	camera->u.asymmetric.near = near;
}

static inline void nemocook_camera_set_asymmetric_far(struct cookcamera *camera, float far)
{
	camera->u.asymmetric.far = far;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
