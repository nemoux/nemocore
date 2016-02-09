#ifndef __NEMOUX_ATOM_MISC_H__
#define __NEMOUX_ATOM_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern const char *simple_vertex_shader;
extern const char *simple_fragment_shader;
extern const char *texture_fragment_shader;

extern GLuint nemoback_atom_create_shader(const char *fshader, const char *vshader);
extern void nemoback_atom_prepare_shader(struct atomback *atom, GLuint program);

extern void nemoback_atom_create_buffer(struct atomback *atom);
extern void nemoback_atom_prepare_buffer(struct atomback *atom, GLenum mode, float *buffers, int elements);
extern void nemoback_atom_prepare_index(struct atomback *atom, GLenum mode, uint32_t *buffers, int elements);

extern GLuint nemoback_atom_create_texture_from_image(const char *filepath);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
