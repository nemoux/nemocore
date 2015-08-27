#ifndef	__MINIBACK_H__
#define	__MINIBACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemomatrix.h>

struct miniback {
	struct nemotool *tool;

	struct eglcontext *egl;
	struct eglcanvas *eglcanvas;

	struct nemocanvas *canvas;

	struct nemotale *tale;
	struct talenode *node;

	int32_t width, height;

	GLuint fbo, dbo;

	GLuint program;
	GLuint utex0;
	GLuint uprojection;
	GLuint ucolor;

	struct nemomatrix matrix;

	GLuint varray;
	GLuint vbuffer;

	GLuint texture;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
