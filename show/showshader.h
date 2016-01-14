#ifndef __NEMOSHOW_SHADER_H__
#define __NEMOSHOW_SHADER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NONE_SHADER = 0,
	NEMOSHOW_LINEAR_GRADIENT_SHADER = 1,
	NEMOSHOW_RADIAL_GRADIENT_SHADER = 2,
	NEMOSHOW_LAST_SHADER
} NemoShowGradientType;

struct showstop {
	struct showone base;

	double offset;

	uint32_t fill;
	double fills[4];
};

struct showshader {
	struct showone base;

	struct showone *ref;

	double x0, y0;
	double x1, y1;
	double r;

	void *cc;
};

#define NEMOSHOW_STOP(one)					((struct showstop *)container_of(one, struct showstop, base))
#define	NEMOSHOW_STOP_AT(one, at)		(NEMOSHOW_STOP(one)->at)

extern struct showone *nemoshow_stop_create(void);
extern void nemoshow_stop_destroy(struct showone *one);

extern void nemoshow_stop_attach_one(struct showone *parent, struct showone *one);
extern void nemoshow_stop_detach_one(struct showone *parent, struct showone *one);

extern int nemoshow_stop_update(struct nemoshow *show, struct showone *one);

static inline void nemoshow_stop_set_fill_color(struct showone *one, double r, double g, double b, double a)
{
	struct showstop *stop = NEMOSHOW_STOP(one);

	stop->fills[0] = r;
	stop->fills[1] = g;
	stop->fills[2] = b;
	stop->fills[3] = a;

	stop->fill = 1;
}

static inline void nemoshow_stop_set_offset(struct showone *one, double offset)
{
	NEMOSHOW_STOP_AT(one, offset) = offset;
}

#define NEMOSHOW_SHADER(one)					((struct showshader *)container_of(one, struct showshader, base))
#define	NEMOSHOW_SHADER_AT(one, at)		(NEMOSHOW_SHADER(one)->at)

extern struct showone *nemoshow_shader_create(int type);
extern void nemoshow_shader_destroy(struct showone *one);

extern int nemoshow_shader_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_shader_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_shader_set_gradient(struct showone *one, const char *mode);

static inline void nemoshow_shader_set_x0(struct showone *one, double x0)
{
	NEMOSHOW_SHADER_AT(one, x0) = x0;
}

static inline void nemoshow_shader_set_y0(struct showone *one, double y0)
{
	NEMOSHOW_SHADER_AT(one, y0) = y0;
}

static inline void nemoshow_shader_set_x1(struct showone *one, double x1)
{
	NEMOSHOW_SHADER_AT(one, x1) = x1;
}

static inline void nemoshow_shader_set_y1(struct showone *one, double y1)
{
	NEMOSHOW_SHADER_AT(one, y1) = y1;
}

static inline void nemoshow_shader_set_r(struct showone *one, double r)
{
	NEMOSHOW_SHADER_AT(one, r) = r;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
