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
	NEMOSHOW_BITMAP_SHADER = 3,
	NEMOSHOW_PERLIN_FRACTAL_NOISE_SHADER = 4,
	NEMOSHOW_PERLIN_TURBULENCE_NOISE_SHADER = 5,
	NEMOSHOW_LAST_SHADER
} NemoShowGradientType;

typedef enum {
	NEMOSHOW_SHADER_CLAMP_TILEMODE = 0,
	NEMOSHOW_SHADER_REPEAT_TILEMODE = 1,
	NEMOSHOW_SHADER_MIRROR_TILEMODE = 2,
	NEMOSHOW_SHADER_LAST_TILEMODE
} NemoShowShaderTileMode;

struct showstop {
	struct showone base;

	double offset;

	double fills[4];
};

struct showshader {
	struct showone base;

	double x0, y0;
	double x1, y1;
	double r;

	uint32_t tmx, tmy;

	uint32_t octaves;
	double seed;

	void *cc;
};

#define NEMOSHOW_STOP(one)					((struct showstop *)container_of(one, struct showstop, base))
#define	NEMOSHOW_STOP_AT(one, at)		(NEMOSHOW_STOP(one)->at)

extern struct showone *nemoshow_stop_create(void);
extern void nemoshow_stop_destroy(struct showone *one);

extern int nemoshow_stop_update(struct showone *one);

static inline void nemoshow_stop_set_color(struct showone *one, double r, double g, double b, double a)
{
	struct showstop *stop = NEMOSHOW_STOP(one);

	stop->fills[NEMOSHOW_RED_COLOR] = r;
	stop->fills[NEMOSHOW_GREEN_COLOR] = g;
	stop->fills[NEMOSHOW_BLUE_COLOR] = b;
	stop->fills[NEMOSHOW_ALPHA_COLOR] = a;

	nemoshow_one_set_state(one, NEMOSHOW_FILL_STATE);

	nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
}

static inline void nemoshow_stop_set_offset(struct showone *one, double offset)
{
	NEMOSHOW_STOP_AT(one, offset) = offset;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

#define NEMOSHOW_SHADER(one)					((struct showshader *)container_of(one, struct showshader, base))
#define	NEMOSHOW_SHADER_AT(one, at)		(NEMOSHOW_SHADER(one)->at)

extern struct showone *nemoshow_shader_create(int type);
extern void nemoshow_shader_destroy(struct showone *one);

extern int nemoshow_shader_update(struct showone *one);

extern void nemoshow_shader_set_shader(struct showone *one, struct showone *shader);
extern void nemoshow_shader_set_bitmap(struct showone *one, struct showone *bitmap);

static inline void nemoshow_shader_set_x0(struct showone *one, double x0)
{
	NEMOSHOW_SHADER_AT(one, x0) = x0;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_shader_set_y0(struct showone *one, double y0)
{
	NEMOSHOW_SHADER_AT(one, y0) = y0;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_shader_set_x1(struct showone *one, double x1)
{
	NEMOSHOW_SHADER_AT(one, x1) = x1;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_shader_set_y1(struct showone *one, double y1)
{
	NEMOSHOW_SHADER_AT(one, y1) = y1;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_shader_set_r(struct showone *one, double r)
{
	NEMOSHOW_SHADER_AT(one, r) = r;

	nemoshow_one_dirty(one, NEMOSHOW_SHAPE_DIRTY);
}

static inline void nemoshow_shader_set_tilemode(struct showone *one, uint32_t tm)
{
	NEMOSHOW_SHADER_AT(one, tmx) = tm;

	nemoshow_one_dirty(one, NEMOSHOW_SHADER_DIRTY);
}

static inline void nemoshow_shader_set_tilemode_x(struct showone *one, uint32_t tm)
{
	NEMOSHOW_SHADER_AT(one, tmx) = tm;

	nemoshow_one_dirty(one, NEMOSHOW_SHADER_DIRTY);
}

static inline void nemoshow_shader_set_tilemode_y(struct showone *one, uint32_t tm)
{
	NEMOSHOW_SHADER_AT(one, tmy) = tm;

	nemoshow_one_dirty(one, NEMOSHOW_SHADER_DIRTY);
}

static inline void nemoshow_shader_set_octaves(struct showone *one, uint32_t octaves)
{
	NEMOSHOW_SHADER_AT(one, octaves) = octaves;

	nemoshow_one_dirty(one, NEMOSHOW_SHADER_DIRTY);
}

static inline void nemoshow_shader_set_seed(struct showone *one, double seed)
{
	NEMOSHOW_SHADER_AT(one, seed) = seed;

	nemoshow_one_dirty(one, NEMOSHOW_SHADER_DIRTY);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
