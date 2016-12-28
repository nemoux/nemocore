#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <cookfbo.h>
#include <nemocook.h>
#include <nemomisc.h>
#include <glhelper.h>

struct cookfbo {
	struct cookone one;

	struct cookshader *shader;

	GLuint texture;
	GLuint fbo, dbo;

	int width, height;
};

int nemocook_fbo_bind(struct cookfbo *fbo)
{
	fbo->shader = NULL;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

	glViewport(0, 0, fbo->width, fbo->height);

	return 0;
}

int nemocook_fbo_unbind(struct cookfbo *fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}

struct cookshader *nemocook_fbo_use_shader(struct cookfbo *fbo, struct cookshader *shader)
{
	if (fbo->shader != shader) {
		nemocook_shader_use_program(shader);

		fbo->shader = shader;
	}

	return fbo->shader;
}

int nemocook_fbo_resize(struct cookfbo *fbo, int width, int height)
{
	if (fbo->width != width || fbo->height != height) {
		glDeleteFramebuffers(1, &fbo->fbo);
		glDeleteRenderbuffers(1, &fbo->dbo);

		if (gl_create_fbo(fbo->texture, width, height, &fbo->fbo, &fbo->dbo) < 0)
			return -1;

		fbo->width = width;
		fbo->height = height;
	}

	return 0;
}

struct cookfbo *nemocook_fbo_create(GLuint texture, GLuint width, GLuint height)
{
	struct cookfbo *fbo;

	fbo = (struct cookfbo *)malloc(sizeof(struct cookfbo));
	if (fbo == NULL)
		return NULL;
	memset(fbo, 0, sizeof(struct cookfbo));

	fbo->texture = texture;
	fbo->width = width;
	fbo->height = height;

	if (gl_create_fbo(fbo->texture, width, height, &fbo->fbo, &fbo->dbo) < 0)
		goto err1;

	nemocook_one_prepare(&fbo->one);

	return fbo;

err1:
	free(fbo);

	return NULL;
}

void nemocook_fbo_destroy(struct cookfbo *fbo)
{
	nemocook_one_finish(&fbo->one);

	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteRenderbuffers(1, &fbo->dbo);

	free(fbo);
}

void nemocook_fbo_attach_state(struct cookfbo *fbo, struct cookstate *state)
{
	nemocook_one_attach_state(&fbo->one, state);
}

void nemocook_fbo_detach_state(struct cookfbo *fbo, int tag)
{
	nemocook_one_detach_state(&fbo->one, tag);
}

void nemocook_fbo_update_state(struct cookfbo *fbo)
{
	nemocook_one_update_state(&fbo->one);
}
