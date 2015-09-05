#ifndef	__MESHBACK_H__
#define	__MESHBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemomatrix.h>

struct meshback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	GLuint fbo, dbo;

	GLuint program;
	GLuint uprojection;
	GLuint ucolor;

	struct nemomatrix matrix;

	GLuint varray;
	GLuint vbuffer;
	GLuint vindex;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
