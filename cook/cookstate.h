#ifndef __NEMOCOOK_STATE_H__
#define __NEMOCOOK_STATE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <nemolist.h>

typedef enum {
	NEMOCOOK_STATE_NONE_TYPE = 0,
	NEMOCOOK_STATE_COLOR_BUFFER_TYPE = 1,
	NEMOCOOK_STATE_DEPTH_BUFFER_TYPE = 2,
	NEMOCOOK_STATE_BLEND_TYPE = 3,
	NEMOCOOK_STATE_BLEND_SEPARATE_TYPE = 4,
	NEMOCOOK_STATE_DEPTH_TEST_TYPE = 5,
	NEMOCOOK_STATE_CULL_FACE_TYPE = 6,
	NEMOCOOK_STATE_POINT_SMOOTH_TYPE = 7,
	NEMOCOOK_STATE_LAST_TYPE
} NemoCookStateType;

struct cookstate;

typedef void (*nemocook_state_update_t)(struct cookstate *state);

struct cookstate {
	struct nemolist link;

	int tag;

	union {
		struct {
			GLfloat r, g, b, a;
		} color_buffer;

		struct {
			GLfloat d;
		} depth_buffer;

		struct {
			GLenum sfactor;
			GLenum dfactor;
		} blend;

		struct {
			GLenum srgb;
			GLenum drgb;
			GLenum salpha;
			GLenum dalpha;
		} blend_separate;

		struct {
			GLenum func;
			GLboolean mask;
		} depth_test;

		struct {
			GLenum mode;
		} cull_face;

		struct {
			GLenum hint;
		} point_smooth;
	} u;

	nemocook_state_update_t update;
};

extern struct cookstate *nemocook_state_create(int tag, int type, ...);
extern void nemocook_state_destroy(struct cookstate *state);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
