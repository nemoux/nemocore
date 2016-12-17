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
	int i;

	nemocook_backend_prerender(cook);

	glViewport(0, 0, cook->width, cook->height);

	nemocook_one_update(&cook->one);

	nemolist_for_each(poly, &cook->poly_list, link) {
		nemocook_renderer_use_shader(renderer, poly->shader);
		nemocook_one_update(&poly->one);

		glUniformMatrix4fv(renderer->shader->utransform, 1, GL_FALSE, poly->matrix.d);

		for (i = 0; i < renderer->shader->nattribs; i++) {
			glVertexAttribPointer(i,
					renderer->shader->attribs[i],
					GL_FLOAT,
					GL_FALSE,
					renderer->shader->attribs[i] * sizeof(GLfloat),
					poly->buffers[i]);
			glEnableVertexAttribArray(i);
		}

		if (poly->texture != NULL) {
			glBindTexture(GL_TEXTURE_2D, poly->texture->texture);
			glDrawArrays(poly->type, 0, poly->count);
			glBindTexture(GL_TEXTURE_2D, 0);
		} else {
			glDrawArrays(poly->type, 0, poly->count);
		}
	}

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
