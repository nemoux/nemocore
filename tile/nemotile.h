#ifndef __NEMO_TILE_H__
#define __NEMO_TILE_H__

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
#include <nemolist.h>
#include <playback.h>
#include <glmotion.h>

struct tileone {
	float *vertices;
	float *texcoords;

	int count;

	struct {
		float tx, ty;
		float r;
		float sx, sy;
	} vtransform;

	struct {
		float tx, ty;
		float r;
		float sx, sy;
	} ttransform;

	struct showone *texture;

	struct nemolist link;
};

struct nemotile {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *over;

	struct showone *canvas;

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

	uint32_t width, height;

	struct nemotimer *timer;
	uint32_t timeout;

	GLuint fbo, dbo;
	GLuint programs[2];
	GLuint uvtransform0;
	GLuint uttransform0;
	GLuint utexture0;
	GLuint uvtransform1;
	GLuint ucolor1;

	float linewidth;

	struct nemolist tile_list;

	uint32_t msecs;
};

#endif
