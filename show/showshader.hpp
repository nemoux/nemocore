#ifndef __NEMOSHOW_SHADER_HPP__
#define __NEMOSHOW_SHADER_HPP__

#include <skiaconfig.hpp>

typedef struct _showshader {
	SkShader *shader;
	SkMatrix *matrix;
} showshader_t;

#define NEMOSHOW_SHADER_CC(base, name)				(((showshader_t *)((base)->cc))->name)

#endif
