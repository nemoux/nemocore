#ifndef	__NEMOSHOW_PIPE_H__
#define	__NEMOSHOW_PIPE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <showone.h>

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>

typedef enum {
	NEMOSHOW_NONE_PIPE = 0,
	NEMOSHOW_SIMPLE_PIPE = 1,
	NEMOSHOW_TEXTURE_PIPE = 2,
	NEMOSHOW_LIGHTING_PIPE = 3,
	NEMOSHOW_LAST_PIPE
} NemoShowPipeType;

struct showpipe {
	struct showone base;

	GLuint program;

	GLuint uprojection;
	GLuint umodelview;
	GLuint ucolor;
	GLuint utex0;

	struct nemomatrix projection;
};

#define NEMOSHOW_PIPE(one)					((struct showpipe *)container_of(one, struct showpipe, base))
#define NEMOSHOW_PIPE_AT(one, at)		(NEMOSHOW_PIPE(one)->at)

extern struct showone *nemoshow_pipe_create(int type);
extern void nemoshow_pipe_destroy(struct showone *one);

extern int nemoshow_pipe_arrange(struct showone *one);
extern int nemoshow_pipe_update(struct showone *one);

extern int nemoshow_pipe_dispatch_one(struct showone *canvas, struct showone *one);

extern void nemoshow_canvas_render_pipeline(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
