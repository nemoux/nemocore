#ifndef __NEMOCOOK_TEXTURE_H__
#define __NEMOCOOK_TEXTURE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef enum {
	NEMOCOOK_TEXTURE_BGRA_FORMAT = 0,
	NEMOCOOK_TEXTURE_LUMINANCE_FORMAT = 1,
	NEMOCOOK_TEXTURE_LAST_FORMAT
} NemoCookTextureFormat;

typedef enum {
	NEMOCOOK_TEXTURE_LINEAR_FILTER = 0,
	NEMOCOOK_TEXTURE_NEAREST_FILTER = 1,
	NEMOCOOK_TEXTURE_LAST_FILTER
} NemoCookTextureFilter;

typedef enum {
	NEMOCOOK_TEXTURE_CLAMP_TO_EDGE_WRAP = 0,
	NEMOCOOK_TEXTURE_CLAMP_TO_BORDER_WRAP = 1,
	NEMOCOOK_TEXTURE_REPEAT_WRAP = 2,
	NEMOCOOK_TEXTURE_MIRRORED_REPEAT_WRAP = 3,
	NEMOCOOK_TEXTURE_LAST_WRAP
} NemoCookTextureWrap;

struct cooktex {
	GLuint texture;
	GLuint format;
	int width, height;
	int bpp;

	GLuint pbo;
};

extern struct cooktex *nemocook_texture_create(int format, int width, int height);
extern void nemocook_texture_destroy(struct cooktex *tex);

extern void nemocook_texture_resize(struct cooktex *tex, int width, int height);

extern void nemocook_texture_set_filter(struct cooktex *tex, int filter);
extern void nemocook_texture_set_wrap(struct cooktex *tex, int wrap);

extern void nemocook_texture_upload(struct cooktex *tex, void *buffer);
extern void nemocook_texture_upload_slice(struct cooktex *tex, void *buffer, int x, int y, int w, int h);

extern void *nemocook_texture_map(struct cooktex *tex);
extern void nemocook_texture_unmap(struct cooktex *tex);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
