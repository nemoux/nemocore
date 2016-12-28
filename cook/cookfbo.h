#ifndef __NEMOCOOK_FBO_H__
#define __NEMOCOOK_FBO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

struct cookfbo;
struct cookstate;
struct cookshader;

extern struct cookfbo *nemocook_fbo_create(GLuint texture, GLuint width, GLuint height);
extern void nemocook_fbo_destroy(struct cookfbo *fbo);

extern int nemocook_fbo_resize(struct cookfbo *fbo, int width, int height);

extern int nemocook_fbo_bind(struct cookfbo *fbo);
extern int nemocook_fbo_unbind(struct cookfbo *fbo);

extern struct cookshader *nemocook_fbo_use_shader(struct cookfbo *fbo, struct cookshader *shader);

extern void nemocook_fbo_attach_state(struct cookfbo *fbo, struct cookstate *state);
extern void nemocook_fbo_detach_state(struct cookfbo *fbo, int tag);
extern void nemocook_fbo_update_state(struct cookfbo *fbo);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
