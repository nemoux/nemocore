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

#include <cookshader.h>
#include <cooktex.h>
#include <cooktrans.h>

#include <nemomatrix.h>
#include <nemolist.h>

struct cookpoly {
	float *buffers[NEMOCOOK_SHADER_ATTRIBS_MAX];
	int elements[NEMOCOOK_SHADER_ATTRIBS_MAX];

	int count;
	int type;

	struct cookshader *shader;

	struct cooktex *texture;

	struct cooktrans *transform;

	struct nemomatrix matrix;

	struct nemolist link;
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

extern void nemocook_polygon_set_transform(struct cookpoly *poly, struct cooktrans *trans);
extern int nemocook_polygon_update_transform(struct cookpoly *poly);

extern void nemocook_polygon_set_shader(struct cookpoly *poly, struct cookshader *shader);

static inline void nemocook_polygon_set_element(struct cookpoly *poly, int attrib, int index, int element, float value)
{
	poly->buffers[attrib][index * poly->elements[attrib] + element] = value;
}

static inline float nemocook_polygon_get_element(struct cookpoly *poly, int attrib, int index, int element)
{
	return poly->buffers[attrib][index * poly->elements[attrib] + element];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
