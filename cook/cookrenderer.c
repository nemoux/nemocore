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
};

static int nemocook_renderer_render(struct nemocook *cook)
{
	struct cookrenderer *renderer = (struct cookrenderer *)cook->context;
	struct cookshader *shader = NULL;
	struct cookpoly *poly;
	int i;

	nemocook_backend_prerender(cook);

	glViewport(0, 0, cook->width, cook->height);

	nemocook_one_update(&cook->one);

	nemolist_for_each(poly, &cook->poly_list, link) {
		if (shader != poly->shader) {
			nemocook_shader_use_program(poly->shader);

			shader = poly->shader;
		}

		glUniformMatrix4fv(shader->utransform, 1, GL_FALSE, poly->matrix.d);

		for (i = 0; i < shader->nattribs; i++) {
			glVertexAttribPointer(i, shader->attribs[i], GL_FLOAT, GL_FALSE, shader->attribs[i] * sizeof(GLfloat), poly->buffers[i]);
			glEnableVertexAttribArray(i);
		}

		nemocook_one_update(&poly->one);

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
