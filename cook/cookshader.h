#ifndef __NEMOCOOK_SHADER_H__
#define __NEMOCOOK_SHADER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define NEMOCOOK_SHADER_ATTRIBS_MAX					(4)
#define NEMOCOOK_SHADER_UNIFORMS_MAX				(16)

struct cookshader {
	GLuint program;
	GLuint vshader;
	GLuint fshader;

	int attribs[NEMOCOOK_SHADER_ATTRIBS_MAX];
	int nattribs;

	GLint uniforms[NEMOCOOK_SHADER_UNIFORMS_MAX];

	uint32_t flags;
};

extern struct cookshader *nemocook_shader_create(void);
extern void nemocook_shader_destroy(struct cookshader *shader);

extern int nemocook_shader_set_program(struct cookshader *shader, const char *vertex_source, const char *fragment_source);
extern void nemocook_shader_use_program(struct cookshader *shader);

extern void nemocook_shader_set_attrib(struct cookshader *shader, int index, const char *name, int size);
extern void nemocook_shader_set_attrib_pointer(struct cookshader *shader, int index, float *pointer);
extern void nemocook_shader_put_attrib_pointer(struct cookshader *shader, int index);

extern void nemocook_shader_set_uniform(struct cookshader *shader, int index, const char *name);
extern void nemocook_shader_set_uniform_1i(struct cookshader *shader, int index, int value);
extern void nemocook_shader_set_uniform_1f(struct cookshader *shader, int index, float value);
extern void nemocook_shader_set_uniform_2fv(struct cookshader *shader, int index, float *value);
extern void nemocook_shader_set_uniform_4fv(struct cookshader *shader, int index, float *value);
extern void nemocook_shader_set_uniform_matrix4fv(struct cookshader *shader, int index, float *value);

static inline int nemocook_shader_get_attribs_count(struct cookshader *shader)
{
	return shader->nattribs;
}

static inline void nemocook_shader_set_flags(struct cookshader *shader, uint32_t flags)
{
	shader->flags |= flags;
}

static inline void nemocook_shader_put_flags(struct cookshader *shader, uint32_t flags)
{
	shader->flags &= ~flags;
}

static inline int nemocook_shader_has_flags(struct cookshader *shader, uint32_t flags)
{
	return (shader->flags & flags) == flags;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
