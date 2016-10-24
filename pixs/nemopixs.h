#ifndef __NEMO_PIXS_H__
#define __NEMO_PIXS_H__

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

struct nemopixs {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *canvas;

	struct showone *sprites[128];
	int nsprites, isprites;

	uint32_t width, height;

	struct nemotimer *timer;
	uint32_t timeout;

	GLuint fbo, dbo;
	GLuint program;

	float *vertices;
	float *velocities;

	float *diffuses;
	float *noises;

	float *vertices0;
	float *diffuses0;

	int is_vertices_dirty;
	int is_diffuses_dirty;

	int pixels;
	int rows, columns;

	float gravitywell_minimum_distance;
	float move_minimum_distance;

	uint32_t msecs;

	float taps[128];
	int ntaps;
};

#endif
