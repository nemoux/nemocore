#ifndef __NEMOUX_ATOM_BACK_H__
#define __NEMOUX_ATOM_BACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>

#include <showhelper.h>

struct atomback {
	struct nemotool *tool;

	struct nemotimer *timer;

	int32_t width, height;
	float aspect;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas0;
	struct showone *canvas9;
	struct showone *inner, *outer, *solid;
	struct showone *ease0;
	struct showone *ease1;
	struct showone *ease2;

	struct showone *pipe;
	struct showone *one;

	struct showone *canvast;
	struct showone *onet;

	GLuint fbo, dbo;
	GLuint program;
	GLuint umatrix;
	GLuint ucolor;
	GLuint utex0;
	struct nemomatrix matrix;

	GLuint varray;
	GLuint vbuffer;
	GLuint vindex;
	GLenum mode;
	int elements;

	int is_sleeping;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
