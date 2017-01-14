#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <drm/drm_fourcc.h>

#include <glprivate.h>

#include <glrenderer.h>
#include <renderer.h>
#include <compz.h>
#include <screen.h>
#include <canvas.h>
#include <actor.h>
#include <view.h>
#include <content.h>
#include <waylandhelper.h>
#include <nemomisc.h>
#include <nemolog.h>

static void glrenderer_prepare_textures(struct glcontent *glcontent, int count)
{
	int i;

	if (count <= glcontent->ntextures)
		return;

	for (i = glcontent->ntextures; i < count; i++) {
		glGenTextures(1, &glcontent->textures[i]);
		glBindTexture(glcontent->target, glcontent->textures[i]);
		glTexParameteri(glcontent->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(glcontent->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	glcontent->ntextures = count;

	glBindTexture(glcontent->target, 0);
}

static void glrenderer_finish_actor_content(struct glrenderer *renderer, struct glcontent *glcontent)
{
	int i;

	wl_list_remove(&glcontent->actor_destroy_listener.link);
	wl_list_remove(&glcontent->renderer_destroy_listener.link);

	nemocontent_set_opengl_context(glcontent->content, renderer->base.node, NULL);

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	glDeleteTextures(glcontent->ntextures, glcontent->textures);

	for (i = 0; i < glcontent->nimages; i++)
		renderer->destroy_image(renderer->egl_display, glcontent->images[i]);

	if (glcontent->image) {
		pixman_image_unref(glcontent->image);
	}

	pixman_region32_fini(&glcontent->damage);

	free(glcontent);
}

static void glrenderer_handle_actor_destroy(struct wl_listener *listener, void *data)
{
	struct glcontent *glcontent = (struct glcontent *)container_of(listener, struct glcontent, actor_destroy_listener);

	glrenderer_finish_actor_content(glcontent->renderer, glcontent);
}

static void glrenderer_handle_actor_renderer_destroy(struct wl_listener *listener, void *data)
{
	struct glcontent *glcontent = (struct glcontent *)container_of(listener, struct glcontent, renderer_destroy_listener);

	glrenderer_finish_actor_content(glcontent->renderer, glcontent);
}

static struct glcontent *glrenderer_prepare_actor_content(struct glrenderer *renderer, struct nemoactor *actor)
{
	struct glcontent *glcontent;

	glcontent = (struct glcontent *)malloc(sizeof(struct glcontent));
	if (glcontent == NULL)
		return NULL;
	memset(glcontent, 0, sizeof(struct glcontent));

	glcontent->pitch = 1;
	glcontent->y_inverted = 1;

	glcontent->renderer = renderer;
	glcontent->content = &actor->base;

	pixman_region32_init(&glcontent->damage);

	glcontent->actor_destroy_listener.notify = glrenderer_handle_actor_destroy;
	wl_signal_add(&actor->destroy_signal, &glcontent->actor_destroy_listener);

	glcontent->renderer_destroy_listener.notify = glrenderer_handle_actor_renderer_destroy;
	wl_signal_add(&renderer->destroy_signal, &glcontent->renderer_destroy_listener);

	return glcontent;
}

static int glrenderer_attach_shm(struct glrenderer *renderer, struct glcontent *glcontent, struct nemoactor *actor)
{
	GLenum format, pixeltype;
	int width, height, stride, pitch;

	if (glcontent->image != NULL) {
		pixman_image_unref(glcontent->image);
		glcontent->image = NULL;
	}

	glcontent->image = pixman_image_ref(actor->image);

	width = pixman_image_get_width(actor->image);
	height = pixman_image_get_height(actor->image);
	stride = pixman_image_get_stride(actor->image);
	pitch = stride / 4;

	glcontent->shader = &renderer->texture_shader_rgba;
	format = GL_BGRA;
	pixeltype = GL_UNSIGNED_BYTE;

	if (pitch != glcontent->pitch ||
			height != glcontent->height ||
			format != glcontent->format ||
			pixeltype != glcontent->pixeltype ||
			glcontent->buffer_type != GL_BUFFER_SHM_TYPE) {
		glcontent->pitch = pitch;
		glcontent->height = height;
		glcontent->target = GL_TEXTURE_2D;
		glcontent->format = format;
		glcontent->pixeltype = pixeltype;
		glcontent->buffer_type = GL_BUFFER_SHM_TYPE;
		glcontent->needs_full_upload = 1;
		glcontent->y_inverted = 1;

		glrenderer_prepare_textures(glcontent, 1);
	}

	return 0;
}

void glrenderer_attach_actor(struct nemorenderer *base, struct nemoactor *actor)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct nemocompz *compz = actor->compz;
	struct glcontent *glcontent;

	glcontent = (struct glcontent *)nemocontent_get_opengl_context(&actor->base, base->node);
	if (glcontent == NULL) {
		glcontent = glrenderer_prepare_actor_content(renderer, actor);
		nemocontent_set_opengl_context(&actor->base, base->node, glcontent);
	}

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	if (actor->image != NULL) {
		glrenderer_attach_shm(renderer, glcontent, actor);
	} else {
		glcontent->pitch = actor->base.width;
		glcontent->height = actor->base.height;
		glcontent->target = GL_TEXTURE_2D;
		glcontent->shader = &renderer->texture_shader_rgba;
		glcontent->format = GL_BGRA;
		glcontent->pixeltype = GL_UNSIGNED_BYTE;
		glcontent->textures[0] = actor->texture;
		glcontent->ntextures = 1;
		glcontent->y_inverted = 1;
	}
}

void glrenderer_flush_actor(struct nemorenderer *base, struct nemoactor *actor)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(&actor->base, base->node);
	pixman_box32_t *boxes;
	uint32_t *data;
	int width, height;
	int i, n;

	if (glcontent == NULL || actor->image == NULL)
		return;

	pixman_region32_union(&glcontent->damage, &glcontent->damage, &actor->base.damage);

	if (!pixman_region32_not_empty(&glcontent->damage) &&
			!glcontent->needs_full_upload)
		goto out;

	width = pixman_image_get_width(actor->image);
	height = pixman_image_get_height(actor->image);
	data = pixman_image_get_data(actor->image);

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	glBindTexture(GL_TEXTURE_2D, glcontent->textures[0]);

	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, glcontent->pitch);

	if (glcontent->needs_full_upload) {
		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, glcontent->format,
				glcontent->pitch, height, 0,
				glcontent->format, glcontent->pixeltype, (void *)data);
	} else {
		boxes = pixman_region32_rectangles(&glcontent->damage, &n);

		for (i = 0; i < n; i++) {
			pixman_box32_t box = nemocontent_transform_to_buffer_rect(&actor->base, boxes[i]);

			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, box.x1);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, box.y1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, box.x1, box.y1,
					box.x2 - box.x1, box.y2 - box.y1,
					glcontent->format, glcontent->pixeltype, (void *)data);
		}
	}

	glBindTexture(GL_TEXTURE_2D, 0);

out:
	pixman_region32_fini(&glcontent->damage);
	pixman_region32_init(&glcontent->damage);
	glcontent->needs_full_upload = 0;
}

int glrenderer_read_actor(struct nemorenderer *base, struct nemoactor *actor, pixman_format_code_t format, void *pixels)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(&actor->base, base->node);
	GLenum glformat;

	if (glcontent == NULL)
		return -1;

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return -1;
	}

	switch (format) {
		case PIXMAN_a8r8g8b8:
			glformat = GL_BGRA;
			break;

		case PIXMAN_a8b8g8r8:
			glformat = GL_RGBA;
			break;

		default:
			return -1;
	}

	glBindTexture(GL_TEXTURE_2D, glcontent->textures[0]);

	glGetTexImage(GL_TEXTURE_2D, 0, glformat, GL_UNSIGNED_BYTE, pixels);

	glBindTexture(GL_TEXTURE_2D, 0);

	return 0;
}
