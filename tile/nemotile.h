#ifndef __NEMOTILE_H__
#define __NEMOTILE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

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
#include <glmask.h>

struct tileone {
	int index;

	float *vertices;
	float *texcoords;
	float *diffuses;
	float *normals;

	float color[4];

	int has_diffuses;

	int count;
	int type;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
	} gtransform0;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
	} gtransform;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
	} vtransform0;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
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
	struct showone *wall;
	struct showone *test;

	struct showone *canvas;

	struct glfilter *filter;
	struct glmask *mask;

	int flip;
	int slideshow;

	int iactions;

	int is_single;
	int is_3d;
	int is_lighting;
	int is_dynamic_perspective;

	struct showone *sprites[128];
	int nsprites, isprites;
	int csprites, rsprites;

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
	GLuint programs[4];
	GLuint uprojection0;
	GLuint uvtransform0;
	GLuint uttransform0;
	GLuint utexture0;
	GLuint ucolor0;
	GLuint uprojection1;
	GLuint uvtransform1;
	GLuint uttransform1;
	GLuint utexture1;
	GLuint uambient1;
	GLuint ulight1;
	GLuint uprojection2;
	GLuint uvtransform2;
	GLuint ucolor2;
	GLuint uprojection3;
	GLuint uvtransform3;
	GLuint uambient3;
	GLuint ulight3;

	struct {
		float tx, ty, tz;
		float rx, ry, rz;
		float sx, sy, sz;
	} projection;

	struct {
		float left, right;
		float bottom, top;
		float near, far;
	} perspective;

	struct {
		float a[3], b[3], c[3], e[3];
		float near, far;
	} asymmetric;

	float light[4];
	float ambient[4];

	float brightness;
	float jitter;

	struct transgroup *trans_group;

	struct nemolist tile_list;
	struct nemolist wall_list;
	struct nemolist test_list;

	struct tileone **tiles;
	int ntiles;

	struct tileone *mesh;
	struct tileone *cube;
	struct tileone *pone;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
