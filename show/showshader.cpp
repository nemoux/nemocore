#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showshader.h>
#include <showshader.hpp>
#include <showitem.h>
#include <showitem.hpp>
#include <nemoshow.h>
#include <nemoxml.h>
#include <nemomisc.h>

struct showone *nemoshow_stop_create(void)
{
	struct showstop *stop;
	struct showone *one;

	stop = (struct showstop *)malloc(sizeof(struct showstop));
	if (stop == NULL)
		return NULL;
	memset(stop, 0, sizeof(struct showstop));

	one = &stop->base;
	one->type = NEMOSHOW_STOP_TYPE;
	one->update = nemoshow_stop_update;
	one->destroy = nemoshow_stop_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "offset", &stop->offset, sizeof(double));
	nemoobject_set_reserved(&one->object, "fill", &stop->fills, sizeof(double[4]));

	nemoshow_one_set_state(one, NEMOSHOW_EFFECT_STATE);
	nemoshow_one_set_effect(one, NEMOSHOW_SHADER_DIRTY);

	return one;
}

void nemoshow_stop_destroy(struct showone *one)
{
	struct showstop *stop = NEMOSHOW_STOP(one);

	nemoshow_one_finish(one);

	free(stop);
}

int nemoshow_stop_update(struct showone *one)
{
	struct showstop *stop = NEMOSHOW_STOP(one);

	return 0;
}

struct showone *nemoshow_shader_create(int type)
{
	struct showshader *shader;
	struct showone *one;

	shader = (struct showshader *)malloc(sizeof(struct showshader));
	if (shader == NULL)
		return NULL;
	memset(shader, 0, sizeof(struct showshader));

	shader->cc = new showshader_t;
	NEMOSHOW_SHADER_CC(shader, matrix) = new SkMatrix;
	NEMOSHOW_SHADER_CC(shader, shader) = NULL;

	one = &shader->base;
	one->type = NEMOSHOW_SHADER_TYPE;
	one->sub = type;
	one->update = nemoshow_shader_update;
	one->destroy = nemoshow_shader_destroy;

	nemoshow_one_prepare(one);

	nemoobject_set_reserved(&one->object, "x", &shader->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y", &shader->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x0", &shader->x0, sizeof(double));
	nemoobject_set_reserved(&one->object, "y0", &shader->y0, sizeof(double));
	nemoobject_set_reserved(&one->object, "x1", &shader->x1, sizeof(double));
	nemoobject_set_reserved(&one->object, "y1", &shader->y1, sizeof(double));
	nemoobject_set_reserved(&one->object, "r", &shader->r, sizeof(double));
	nemoobject_set_reserved(&one->object, "tilemode", &shader->tmx, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "tilemode-x", &shader->tmx, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "tilemode-y", &shader->tmy, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "octaves", &shader->octaves, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "seed", &shader->seed, sizeof(double));

	return one;
}

void nemoshow_shader_destroy(struct showone *one)
{
	struct showshader *shader = NEMOSHOW_SHADER(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_SHADER_CC(shader, matrix) != NULL)
		delete NEMOSHOW_SHADER_CC(shader, matrix);

	delete static_cast<showshader_t *>(shader->cc);

	free(shader);
}

int nemoshow_shader_update(struct showone *one)
{
	static const SkShader::TileMode tilemodes[] = {
		SkShader::kClamp_TileMode,
		SkShader::kRepeat_TileMode,
		SkShader::kMirror_TileMode
	};
	struct showshader *shader = NEMOSHOW_SHADER(one);

	if ((one->dirty & (NEMOSHOW_SHADER_DIRTY | NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_BITMAP_DIRTY)) != 0) {
		if (one->sub == NEMOSHOW_LINEAR_GRADIENT_SHADER || one->sub == NEMOSHOW_RADIAL_GRADIENT_SHADER) {
			struct showone *ref = NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF);
			struct showstop *stop;
			SkColor colors[32];
			SkScalar offsets[32];
			struct showone *child;
			int noffsets = 0;

			if (ref != NULL) {
				nemoshow_children_for_each(child, ref) {
					stop = NEMOSHOW_STOP(child);

					colors[noffsets] = SkColorSetARGB(
							stop->fills[NEMOSHOW_ALPHA_COLOR],
							stop->fills[NEMOSHOW_RED_COLOR],
							stop->fills[NEMOSHOW_GREEN_COLOR],
							stop->fills[NEMOSHOW_BLUE_COLOR]);
					offsets[noffsets] = stop->offset;

					noffsets++;
				}
			} else {
				nemoshow_children_for_each(child, one) {
					stop = NEMOSHOW_STOP(child);

					colors[noffsets] = SkColorSetARGB(
							stop->fills[NEMOSHOW_ALPHA_COLOR],
							stop->fills[NEMOSHOW_RED_COLOR],
							stop->fills[NEMOSHOW_GREEN_COLOR],
							stop->fills[NEMOSHOW_BLUE_COLOR]);
					offsets[noffsets] = stop->offset;

					noffsets++;
				}
			}

			if (one->sub == NEMOSHOW_LINEAR_GRADIENT_SHADER) {
				SkPoint points[] = {
					{ SkDoubleToScalar(shader->x0), SkDoubleToScalar(shader->y0) },
					{ SkDoubleToScalar(shader->x1), SkDoubleToScalar(shader->y1) }
				};

				NEMOSHOW_SHADER_CC(shader, shader) = SkGradientShader::MakeLinear(
						points,
						colors,
						offsets,
						noffsets,
						tilemodes[shader->tmx]);
			} else if (one->sub == NEMOSHOW_RADIAL_GRADIENT_SHADER) {
				NEMOSHOW_SHADER_CC(shader, shader) = SkGradientShader::MakeRadial(
						SkPoint::Make(shader->x0, shader->y0),
						shader->r,
						colors,
						offsets,
						noffsets,
						tilemodes[shader->tmx]);
			}
		} else if (one->sub == NEMOSHOW_BITMAP_SHADER) {
			struct showone *ref = NEMOSHOW_REF(one, NEMOSHOW_BITMAP_REF);

			if (ref != NULL) {
				struct showitem *item = NEMOSHOW_ITEM(ref);

				NEMOSHOW_SHADER_CC(shader, shader) = SkShader::MakeBitmapShader(
						*NEMOSHOW_ITEM_CC(item, bitmap),
						tilemodes[shader->tmx],
						tilemodes[shader->tmy]);
			}
		} else if (one->sub == NEMOSHOW_PERLIN_FRACTAL_NOISE_SHADER) {
			NEMOSHOW_SHADER_CC(shader, shader) = SkPerlinNoiseShader::MakeFractalNoise(
					SkDoubleToScalar(shader->x0), SkDoubleToScalar(shader->y0),
					shader->octaves,
					SkDoubleToScalar(shader->seed));
		} else if (one->sub == NEMOSHOW_PERLIN_TURBULENCE_NOISE_SHADER) {
			NEMOSHOW_SHADER_CC(shader, shader) = SkPerlinNoiseShader::MakeTurbulence(
					SkDoubleToScalar(shader->x0), SkDoubleToScalar(shader->y0),
					shader->octaves,
					SkDoubleToScalar(shader->seed));
		}
	}

	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		if (nemoshow_one_has_state(one, NEMOSHOW_TRANSFORM_STATE)) {
			NEMOSHOW_SHADER_CC(shader, shader) = NEMOSHOW_SHADER_CC(shader, shader)->makeWithLocalMatrix(*NEMOSHOW_SHADER_CC(shader, matrix));
		}
	}

	return 0;
}

void nemoshow_shader_set_shader(struct showone *one, struct showone *shader)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_SHADER_REF));
	nemoshow_one_reference_one(one, shader, NEMOSHOW_SHADER_DIRTY, 0x0, NEMOSHOW_SHADER_REF);
}

void nemoshow_shader_set_bitmap(struct showone *one, struct showone *bitmap)
{
	nemoshow_one_unreference_one(one, NEMOSHOW_REF(one, NEMOSHOW_BITMAP_REF));
	nemoshow_one_reference_one(one, bitmap, NEMOSHOW_BITMAP_DIRTY, 0x0, NEMOSHOW_BITMAP_REF);
}
