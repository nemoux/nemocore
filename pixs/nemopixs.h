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
#include <nemoplay.h>
#include <nemofs.h>
#include <playback.h>
#include <glblur.h>
#include <glmotion.h>

struct pixsfence {
	struct showone *canvas;

	uint8_t *pixels;

	int width, height;
};

struct pixsone {
	float *vertices;
	float *velocities;

	float *diffuses;
	float *noises;
	float *sleeps;

	float *vertices0;
	float *diffuses0;
	float *positions0;
	float *pixels0;
	uint32_t pixscount;
	uint32_t pixscount0;

	GLuint varray;
	GLuint vvertex;
	GLuint vdiffuse;

	int is_vertices_dirty;
	int is_diffuses_dirty;
	int is_texcoords_dirty;
	int is_pixels_dirty;
	int is_hidden;

	struct showone *texture;

	int rows, columns;

	float pixsize;
};

struct nemopixs {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *over;

	struct showone *canvas;

	struct glblur *blur;
	struct glmotion *motion;

	struct showone *sprites[128];
	int nsprites, isprites;

	struct fsdir *movies;
	int imovies;

	struct showone *video;
	struct nemoplay *play;
	struct playback_decoder *decoderback;
	struct playback_audio *audioback;
	struct playback_video *videoback;

	int iactions;
	int tapmax;
	int tapidx;

	struct showone *pointsprite;
	struct showone *pointone;

	uint32_t width, height;

	struct nemotimer *stimer;
	uint32_t timeout;

	struct nemotimer *ptimer;

	GLuint fbo, dbo;
	GLuint programs[4];
	GLuint usprite1, usprite3;
	GLuint utexture3;

	struct pixsone *one;

	int pixels;
	float jitter;
	float pixsize;

	uint32_t msecs;

	struct showevent events;
	int has_taps;
};

#endif
