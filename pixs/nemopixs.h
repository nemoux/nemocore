#ifndef __NEMO_PIXS_H__
#define __NEMO_PIXS_H__

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <nemotool.h>
#include <nemoshow.h>

struct nemopixs {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *canvas;

	struct showone *sprite;

	uint32_t width, height;

	GLuint fbo, dbo;
	GLuint program;

	float *vertices;
	float *velocities;
	float *diffuses;

	int pixels;
	int rows, columns;

	uint32_t msecs;

	float taps[128];
	int ntaps;
};

#endif
