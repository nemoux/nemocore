#ifndef	__NEMOUX_MESH_HELPER_H__
#define	__NEMOUX_MESH_HELPER_H__

#include <stdint.h>

#include <GL/gl.h>

#include <nemomatrix.h>

struct nemomesh {
	struct nemomatrix modelview;

	GLuint varray;
	GLuint vbuffer;
	GLuint vindex;

	GLuint garray;
	GLuint gbuffer;

	float *lines;
	int nlines, slines;

	float *meshes;
	int nmeshes, smeshes;

	float *guides;
	int nguides, sguides;

	int on_guides;

	GLenum mode;
	int elements;

	float boundingbox[6];

	float sx, sy, sz;
	float tx, ty, tz;

	struct nemovector avec, cvec;
	struct nemoquaternion squat, cquat;
};

extern struct nemomesh *nemomesh_create_object(const char *filepath, const char *basepath);
extern void nemomesh_destroy_object(struct nemomesh *mesh);

extern void nemomesh_prepare_buffer(struct nemomesh *mesh, GLenum mode, float *buffers, int elements);
extern void nemomesh_prepare_index(struct nemomesh *mesh, GLenum mode, uint32_t *buffers, int elements);

extern void nemomesh_update_transform(struct nemomesh *mesh);

extern void nemomesh_reset_quaternion(struct nemomesh *mesh, int32_t width, int32_t height, float x, float y);
extern void nemomesh_update_quaternion(struct nemomesh *mesh, int32_t width, int32_t height, float x, float y);

static inline void nemomesh_turn_on_guides(struct nemomesh *mesh)
{
	mesh->on_guides = 1;
}

static inline void nemomesh_turn_off_guides(struct nemomesh *mesh)
{
	mesh->on_guides = 0;
}

#endif
