#ifndef __NEMOUX_ATOM_MISC_H__
#define __NEMOUX_ATOM_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern const char *simple_vertex_shader;
extern const char *simple_fragment_shader;

extern GLuint nemoback_atom_create_shader(const char *fshader, const char *vshader);
extern void nemoback_atom_prepare_shader(struct atomback *atom, GLuint program);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
