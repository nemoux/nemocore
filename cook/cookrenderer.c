#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookrenderer.h>
#include <nemocook.h>
#include <nemomisc.h>

struct cookrenderer {
	struct cookshader *shader;
};

static inline void nemocook_renderer_prerender(struct cookrenderer *renderer)
{
	renderer->shader = NULL;
}

static inline void nemocook_renderer_postrender(struct cookrenderer *renderer)
{
}

static inline void nemocook_renderer_use_shader(struct cookrenderer *renderer, struct cookshader *shader)
{
	if (renderer->shader != shader) {
		nemocook_shader_use_program(shader);

		renderer->shader = shader;
	}
}

static int nemocook_renderer_render(struct nemocook *cook)
{
	struct cookrenderer *renderer = (struct cookrenderer *)cook->context;
	struct cookpoly *poly;

	nemocook_backend_prerender(cook);
	nemocook_renderer_prerender(renderer);

	glViewport(0, 0, cook->width, cook->height);

	nemocook_one_update(&cook->one);

	nemolist_for_each(poly, &cook->poly_list, link) {
		nemocook_renderer_use_shader(renderer, poly->shader);

		if (nemocook_shader_has_polygon_uniforms(renderer->shader, NEMOCOOK_SHADER_POLYGON_TRANSFORM_UNIFORM) != 0)
			nemocook_shader_update_polygon_transform(renderer->shader, poly);
		if (nemocook_shader_has_polygon_uniforms(renderer->shader, NEMOCOOK_SHADER_POLYGON_COLOR_UNIFORM) != 0)
			nemocook_shader_update_polygon_color(renderer->shader, poly);

		nemocook_shader_update_polygon_attribs(renderer->shader, poly);

		nemocook_one_update(&poly->one);

		nemocook_polygon_draw(poly);
	}

	nemocook_renderer_postrender(renderer);
	nemocook_backend_postrender(cook);

	return 0;
}

int nemocook_prepare_renderer(struct nemocook *cook)
{
	struct cookrenderer *renderer;

	renderer = (struct cookrenderer *)malloc(sizeof(struct cookrenderer));
	if (renderer == NULL)
		return -1;
	memset(renderer, 0, sizeof(struct cookrenderer));

	cook->render = nemocook_renderer_render;
	cook->context = (void *)renderer;

	return 0;
}
