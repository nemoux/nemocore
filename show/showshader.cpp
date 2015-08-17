#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showshader.h>
#include <showshader.hpp>
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
	nemoobject_set_reserved(&one->object, "fill", &stop->fill, sizeof(uint32_t));
	nemoobject_set_reserved(&one->object, "fill:r", &stop->fills[2], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:g", &stop->fills[1], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:b", &stop->fills[0], sizeof(double));
	nemoobject_set_reserved(&one->object, "fill:a", &stop->fills[3], sizeof(double));

	return one;
}

void nemoshow_stop_destroy(struct showone *one)
{
	struct showstop *stop = NEMOSHOW_STOP(one);

	nemoshow_one_finish(one);

	free(stop);
}

int nemoshow_stop_arrange(struct nemoshow *show, struct showone *one)
{
	struct showstop *stop = NEMOSHOW_STOP(one);

	return 0;
}

int nemoshow_stop_update(struct nemoshow *show, struct showone *one)
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

	return one;
}

void nemoshow_shader_destroy(struct showone *one)
{
	struct showshader *shader = NEMOSHOW_SHADER(one);

	nemoshow_one_finish(one);

	delete static_cast<showshader_t *>(shader->cc);

	free(shader);
}

int nemoshow_shader_arrange(struct nemoshow *show, struct showone *one)
{
	struct showshader *shader = NEMOSHOW_SHADER(one);
	struct showstop *stop;
	SkShader::TileMode f;
	const char *flags = nemoobject_gets(&one->object, "flags");
	int i;

	if (NEMOSHOW_SHADER_CC(shader, shader) != NULL)
		NEMOSHOW_SHADER_CC(shader, shader)->unref();

	if (flags == NULL)
		f = SkShader::kClamp_TileMode;
	else if (strcmp(flags, "tile"))
		f = SkShader::kClamp_TileMode;
	else if (strcmp(flags, "repeat"))
		f = SkShader::kRepeat_TileMode;
	else if (strcmp(flags, "mirror"))
		f = SkShader::kMirror_TileMode;

	SkColor colors[one->nchildren];
	SkScalar offsets[one->nchildren];
	int noffsets;

	if (shader->ref != NULL) {
		for (i = 0; i < shader->ref->nchildren; i++) {
			stop = NEMOSHOW_STOP(shader->ref->children[i]);

			colors[i] = SkColorSetARGB(stop->fills[3], stop->fills[2], stop->fills[1], stop->fills[0]);
			offsets[i] = stop->offset;
		}

		noffsets = shader->ref->nchildren;
	} else {
		for (i = 0; i < one->nchildren; i++) {
			stop = NEMOSHOW_STOP(one->children[i]);

			colors[i] = SkColorSetARGB(stop->fills[3], stop->fills[2], stop->fills[1], stop->fills[0]);
			offsets[i] = stop->offset;
		}

		noffsets = one->nchildren;
	}

	if (one->sub == NEMOSHOW_LINEAR_GRADIENT_SHADER) {
		SkPoint points[] = {
			{ SkDoubleToScalar(shader->x0), SkDoubleToScalar(shader->y0) },
			{ SkDoubleToScalar(shader->x1), SkDoubleToScalar(shader->y1) }
		};

		NEMOSHOW_SHADER_CC(shader, shader) = SkGradientShader::CreateLinear(
				points,
				colors,
				offsets,
				noffsets,
				f);
	} else if (one->sub == NEMOSHOW_RADIAL_GRADIENT_SHADER) {
		NEMOSHOW_SHADER_CC(shader, shader) = SkGradientShader::CreateRadial(
				SkPoint::Make(shader->x0, shader->y0),
				shader->r,
				colors,
				offsets,
				noffsets,
				f);
	}

	return 0;
}

int nemoshow_shader_update(struct nemoshow *show, struct showone *one)
{
	struct showshader *shader = NEMOSHOW_SHADER(one);

	if ((one->dirty & NEMOSHOW_MATRIX_DIRTY) != 0) {
		NEMOSHOW_SHADER_CC(shader, shader) = SkShader::CreateLocalMatrixShader(
				NEMOSHOW_SHADER_CC(shader, shader),
				*NEMOSHOW_SHADER_CC(shader, matrix));
	}

	return 0;
}
