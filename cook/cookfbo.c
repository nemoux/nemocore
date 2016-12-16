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
	GLuint texture;
	GLuint fbo, dbo;
};

static int nemocook_fbo_prerender(struct nemocook *cook)
{
	struct cookfbo *fbo = (struct cookfbo *)cook->backend;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

	return 0;
}

static int nemocook_fbo_postrender(struct nemocook *cook)
{
	struct cookfbo *fbo = (struct cookfbo *)cook->backend;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}

static int nemocook_fbo_resize(struct nemocook *cook, int width, int height)
{
	struct cookfbo *fbo = (struct cookfbo *)cook->backend;

	cook->width = width;
	cook->height = height;

	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteRenderbuffers(1, &fbo->dbo);

	if (gl_create_fbo(fbo->texture, width, height, &fbo->fbo, &fbo->dbo) < 0)
		return -1;

	return 0;
}

static void nemocook_fbo_finish(struct nemocook *cook)
{
	struct cookfbo *fbo = (struct cookfbo *)cook->backend;

	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteRenderbuffers(1, &fbo->dbo);

	free(fbo);
}

int nemocook_prepare_fbo(struct nemocook *cook, GLuint texture, GLuint width, GLuint height)
{
	struct cookfbo *fbo;

	fbo = (struct cookfbo *)malloc(sizeof(struct cookfbo));
	if (fbo == NULL)
		return -1;
	memset(fbo, 0, sizeof(struct cookfbo));

	fbo->texture = texture;

	if (gl_create_fbo(fbo->texture, width, height, &fbo->fbo, &fbo->dbo) < 0)
		goto err1;

	cook->backend_prerender = nemocook_fbo_prerender;
	cook->backend_postrender = nemocook_fbo_postrender;
	cook->backend_resize = nemocook_fbo_resize;
	cook->backend_finish = nemocook_fbo_finish;
	cook->backend = (void *)fbo;

	cook->width = width;
	cook->height = height;

	return 0;

err1:
	free(fbo);

	return -1;
}
