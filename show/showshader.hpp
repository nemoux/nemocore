#ifndef __NEMOSHOW_SHADER_HPP__
#define __NEMOSHOW_SHADER_HPP__

typedef struct _showshader {
	SkShader *shader;
	SkMatrix *matrix;
} showshader_t;

#define NEMOSHOW_SHADER_CC(base, name)				(((showshader_t *)((base)->cc))->name)
#define NEMOSHOW_SHADER_ATCC(one, name)				(NEMOSHOW_SHADER_CC(NEMOSHOW_SHADER(one), name))

#endif
