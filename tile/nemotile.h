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
#include <nemotrans.h>
#include <nemolist.h>
#include <playback.h>
#include <glfilter.h>
#include <glmotion.h>

struct tileone {
	int index;

	float *vertices;
	float *texcoords;

	float color[4];

	int count;

	struct {
		float tx, ty;
		float r;
		float sx, sy;
	} vtransform0;

	struct {
		float tx, ty;
		float r;
		float sx, sy;
	} vtransform;

	struct {
		float tx, ty;
		float r;
		float sx, sy;
	} ttransform0;

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

	struct glfilter *filter;
	struct glmotion *motion;

	int flip;

	int iactions;

	int is_single;
	int is_slideshow;

	struct showone *sprites[128];
	int nsprites, isprites;
	int csprites;

	struct fsdir *movies;
	int imovies;

	struct showone *video;
	struct nemoplay *play;
	struct playback_decoder *decoderback;
	struct playback_audio *audioback;
	struct playback_video *videoback;

	uint32_t width, height;
	uint32_t columns, rows;

	struct nemotimer *timer;
	uint32_t timeout;

	GLuint fbo, dbo;
	GLuint programs[2];
	GLuint uvtransform0;
	GLuint uttransform0;
	GLuint utexture0;
	GLuint ucolor0;
	GLuint uvtransform1;
	GLuint ucolor1;

	float linewidth;
	float brightness;
	float jitter;

	struct transgroup *trans_group;

	struct nemolist tile_list;
	struct tileone *pone;

	uint32_t etime;
};

#endif
