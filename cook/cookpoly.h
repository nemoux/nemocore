#ifndef __NEMOCOOK_POLYGON_H__
#define __NEMOCOOK_POLYGON_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <cooktex.h>
#include <cooktrans.h>
#include <cookone.h>
#include <cookstate.h>
#include <cookshader.h>

#include <nemomatrix.h>
#include <nemolist.h>

struct cookpoly;

typedef void (*nemocook_poly_draw_t)(struct cookpoly *poly);

struct cookpoly {
	struct cookone one;

	float *buffers[NEMOCOOK_SHADER_ATTRIBS_MAX];
	int elements[NEMOCOOK_SHADER_ATTRIBS_MAX];

	int count;
	int type;

	struct cooktex *texture;

	float color[4];

	struct cooktrans *transform;

	struct nemomatrix matrix;

	struct nemolist link;

	nemocook_poly_draw_t draw;
};

extern struct cookpoly *nemocook_polygon_create(void);
extern void nemocook_polygon_destroy(struct cookpoly *poly);

extern void nemocook_polygon_set_count(struct cookpoly *poly, int count);
extern void nemocook_polygon_set_type(struct cookpoly *poly, int type);

extern void nemocook_polygon_set_buffer(struct cookpoly *poly, int attrib, int element);
extern float *nemocook_polygon_get_buffer(struct cookpoly *poly, int attrib);

extern void nemocook_polygon_copy_buffer(struct cookpoly *poly, int attrib, float *buffer, int size);

extern void nemocook_polygon_set_texture(struct cookpoly *poly, struct cooktex *tex);
extern struct cooktex *nemocook_polygon_get_texture(struct cookpoly *poly);

extern void nemocook_polygon_set_color(struct cookpoly *poly, float r, float g, float b, float a);

extern void nemocook_polygon_set_transform(struct cookpoly *poly, struct cooktrans *trans);
extern int nemocook_polygon_update_transform(struct cookpoly *poly);

static inline void nemocook_polygon_attach_state(struct cookpoly *poly, struct cookstate *state)
{
	nemocook_one_attach_state(&poly->one, state);
}

static inline void nemocook_polygon_detach_state(struct cookpoly *poly, int tag)
{
	nemocook_one_detach_state(&poly->one, tag);
}

static inline void nemocook_polygon_update_state(struct cookpoly *poly)
{
	nemocook_one_update_state(&poly->one);
}

static inline void nemocook_polygon_set_element(struct cookpoly *poly, int attrib, int index, int element, float value)
{
	poly->buffers[attrib][index * poly->elements[attrib] + element] = value;
}

static inline float nemocook_polygon_get_element(struct cookpoly *poly, int attrib, int index, int element)
{
	return poly->buffers[attrib][index * poly->elements[attrib] + element];
}

static inline void nemocook_polygon_draw(struct cookpoly *poly)
{
	poly->draw(poly);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif