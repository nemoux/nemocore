#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glprivate.h>

#include <glrenderer.h>
#include <renderer.h>
#include <compz.h>
#include <screen.h>
#include <canvas.h>
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

static int glrenderer_attach_shm(struct glrenderer *renderer, struct glcontent *glcontent, struct nemobuffer *buffer, struct wl_shm_buffer *shmbuffer)
{
	GLenum format, pixeltype;
	int pitch;

	switch (wl_shm_buffer_get_format(shmbuffer)) {
		case WL_SHM_FORMAT_XRGB8888:
			glcontent->shader = &renderer->texture_shader_rgbx;
			pitch = wl_shm_buffer_get_stride(shmbuffer) / 4;
			format = GL_BGRA;
			pixeltype = GL_UNSIGNED_BYTE;
			break;

		case WL_SHM_FORMAT_ARGB8888:
			glcontent->shader = &renderer->texture_shader_rgba;
			pitch = wl_shm_buffer_get_stride(shmbuffer) / 4;
			format = GL_BGRA;
			pixeltype = GL_UNSIGNED_BYTE;
			break;

		case WL_SHM_FORMAT_RGB565:
			glcontent->shader = &renderer->texture_shader_rgbx;
			pitch = wl_shm_buffer_get_stride(shmbuffer) / 2;
			format = GL_RGB;
			pixeltype = GL_UNSIGNED_SHORT_5_6_5;
			break;

		default:
			nemolog_error("GLRENDERER", "unavailable shm buffer format\n");
			return -1;
	}

	if (pitch != glcontent->pitch ||
			buffer->height != glcontent->height ||
			format != glcontent->format ||
			pixeltype != glcontent->pixeltype ||
			glcontent->buffer_type != GL_BUFFER_SHM_TYPE) {
		glcontent->pitch = pitch;
		glcontent->height = buffer->height;
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

static int glrenderer_attach_egl(struct glrenderer *renderer, struct glcontent *glcontent, struct nemobuffer *buffer, uint32_t format)
{
	EGLint attribs[3];
	int i, nplanes;

	for (i = 0; i < glcontent->nimages; i++)
		renderer->destroy_image(renderer->egl_display, glcontent->images[i]);
	glcontent->nimages = 0;
	glcontent->target = GL_TEXTURE_2D;

	switch (format) {
		case EGL_TEXTURE_RGB:
		case EGL_TEXTURE_RGBA:
		default:
			nplanes = 1;
			glcontent->shader = &renderer->texture_shader_rgba;
			break;

		case EGL_TEXTURE_EXTERNAL_WL:
			nplanes = 1;
			glcontent->target = GL_TEXTURE_EXTERNAL_OES;
			glcontent->shader = &renderer->texture_shader_egl_external;
			break;

		case EGL_TEXTURE_Y_UV_WL:
			nplanes = 2;
			glcontent->shader = &renderer->texture_shader_y_uv;
			break;

		case EGL_TEXTURE_Y_U_V_WL:
			nplanes = 2;
			glcontent->shader = &renderer->texture_shader_y_u_v;
			break;

		case EGL_TEXTURE_Y_XUXV_WL:
			nplanes = 2;
			glcontent->shader = &renderer->texture_shader_y_xuxv;
			break;
	}

	glrenderer_prepare_textures(glcontent, nplanes);

	for (i = 0; i < nplanes; i++) {
		attribs[0] = EGL_WAYLAND_PLANE_WL;
		attribs[1] = i;
		attribs[2] = EGL_NONE;

		glcontent->images[i] = renderer->create_image(renderer->egl_display,
				NULL,
				EGL_WAYLAND_BUFFER_WL,
				buffer->legacy_buffer,
				attribs);
		if (glcontent->images[i] == NULL) {
			nemolog_error("GLRENDERER", "failed to create image for plane %d\n", i);
			return -1;
		}
		glcontent->nimages++;

		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(glcontent->target, glcontent->textures[i]);
		renderer->image_target_texture_2d(glcontent->target, glcontent->images[i]);
		glBindTexture(glcontent->target, 0);
	}

	glcontent->pitch = buffer->width;
	glcontent->height = buffer->height;
	glcontent->buffer_type = GL_BUFFER_EGL_TYPE;
	glcontent->y_inverted = buffer->y_inverted;

	return 0;
}

static void glrenderer_finish_canvas_content(struct glrenderer *renderer, struct glcontent *glcontent)
{
	int i;

	wl_list_remove(&glcontent->canvas_destroy_listener.link);
	wl_list_remove(&glcontent->renderer_destroy_listener.link);

	nemocontent_set_opengl_context(glcontent->content, renderer->base.node, NULL);

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	glDeleteTextures(glcontent->ntextures, glcontent->textures);

	for (i = 0; i < glcontent->nimages; i++)
		renderer->destroy_image(renderer->egl_display, glcontent->images[i]);

	nemobuffer_reference(&glcontent->buffer_reference, NULL);
	pixman_region32_fini(&glcontent->damage);

	free(glcontent);
}

static void glrenderer_handle_canvas_destroy(struct wl_listener *listener, void *data)
{
	struct glcontent *glcontent = (struct glcontent *)container_of(listener, struct glcontent, canvas_destroy_listener);

	glrenderer_finish_canvas_content(glcontent->renderer, glcontent);
}

static void glrenderer_handle_renderer_destroy(struct wl_listener *listener, void *data)
{
	struct glcontent *glcontent = (struct glcontent *)container_of(listener, struct glcontent, renderer_destroy_listener);

	glrenderer_finish_canvas_content(glcontent->renderer, glcontent);
}

static struct glcontent *glrenderer_prepare_canvas_content(struct glrenderer *renderer, struct nemocanvas *canvas)
{
	struct glcontent *glcontent;

	glcontent = (struct glcontent *)malloc(sizeof(struct glcontent));
	if (glcontent == NULL)
		return NULL;
	memset(glcontent, 0, sizeof(struct glcontent));

	glcontent->pitch = 1;
	glcontent->y_inverted = 1;

	glcontent->renderer = renderer;
	glcontent->content = &canvas->base;

	pixman_region32_init(&glcontent->damage);

	glcontent->canvas_destroy_listener.notify = glrenderer_handle_canvas_destroy;
	wl_signal_add(&canvas->destroy_signal, &glcontent->canvas_destroy_listener);

	glcontent->renderer_destroy_listener.notify = glrenderer_handle_renderer_destroy;
	wl_signal_add(&renderer->destroy_signal, &glcontent->renderer_destroy_listener);

	return glcontent;
}

void glrenderer_prepare_buffer(struct nemorenderer *base, struct nemobuffer *buffer)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct wl_shm_buffer *shmbuffer;

	shmbuffer = wl_shm_buffer_get(buffer->resource);
	if (shmbuffer) {
		buffer->shmbuffer = shmbuffer;
		buffer->width = wl_shm_buffer_get_width(shmbuffer);
		buffer->height = wl_shm_buffer_get_height(shmbuffer);
	} else {
		if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
			nemolog_error("GLRENDERER", "failed to make egl context current\n");
			return;
		}

		buffer->legacy_buffer = (void *)buffer->resource;
		renderer->query_buffer(renderer->egl_display, buffer->legacy_buffer,
				EGL_WIDTH, &buffer->width);
		renderer->query_buffer(renderer->egl_display, buffer->legacy_buffer,
				EGL_HEIGHT, &buffer->height);
		renderer->query_buffer(renderer->egl_display, buffer->legacy_buffer,
				EGL_WAYLAND_Y_INVERTED_WL, &buffer->y_inverted);
		renderer->query_buffer(renderer->egl_display, buffer->legacy_buffer,
				EGL_TEXTURE_FORMAT, &buffer->format);
	}
}

void glrenderer_attach_canvas(struct nemorenderer *base, struct nemocanvas *canvas)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glcontent *glcontent;
	struct nemobuffer *buffer = canvas->buffer_reference.buffer;
	struct wl_shm_buffer *shmbuffer;
	int i;

	glcontent = (struct glcontent *)nemocontent_get_opengl_context(&canvas->base, base->node);
	if (glcontent == NULL) {
		glcontent = glrenderer_prepare_canvas_content(renderer, canvas);
		nemocontent_set_opengl_context(&canvas->base, base->node, glcontent);
	}

	nemobuffer_reference(&glcontent->buffer_reference, buffer);

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	if (buffer == NULL) {
		for (i = 0; i < glcontent->nimages; i++) {
			renderer->destroy_image(renderer->egl_display, glcontent->images[i]);
			glcontent->images[i] = NULL;
		}

		glcontent->nimages = 0;
		glDeleteTextures(glcontent->ntextures, glcontent->textures);
		glcontent->ntextures = 0;
		glcontent->buffer_type = GL_BUFFER_NULL_TYPE;
		glcontent->y_inverted = 1;
		return;
	}

	shmbuffer = wl_shm_buffer_get(buffer->resource);
	if (shmbuffer && glrenderer_attach_shm(renderer, glcontent, buffer, shmbuffer) == 0) {
	} else if (glrenderer_attach_egl(renderer, glcontent, buffer, buffer->format) == 0) {
	} else {
		nemolog_error("GLRENDERER", "unavailable buffer type\n");

		nemobuffer_reference(&glcontent->buffer_reference, NULL);
		glcontent->buffer_type = GL_BUFFER_NULL_TYPE;
		glcontent->y_inverted = 1;
	}
}

void glrenderer_flush_canvas(struct nemorenderer *base, struct nemocanvas *canvas)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(&canvas->base, base->node);
	struct nemobuffer *buffer;
	pixman_box32_t *boxes;
	void *data;
	int i, n;

	if (glcontent == NULL ||
			glcontent->buffer_reference.buffer == NULL ||
			wl_shm_buffer_get(glcontent->buffer_reference.buffer->resource) == NULL)
		return;

	buffer = glcontent->buffer_reference.buffer;

	pixman_region32_union(&glcontent->damage, &glcontent->damage, &canvas->base.damage);

	if (!pixman_region32_not_empty(&glcontent->damage) &&
			!glcontent->needs_full_upload)
		goto out;

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	glBindTexture(GL_TEXTURE_2D, glcontent->textures[0]);

	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, glcontent->pitch);

	data = wl_shm_buffer_get_data(buffer->shmbuffer);

	if (glcontent->needs_full_upload) {
		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

		wl_shm_buffer_begin_access(buffer->shmbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, glcontent->format,
				glcontent->pitch, buffer->height, 0,
				glcontent->format, glcontent->pixeltype, data);
		wl_shm_buffer_end_access(buffer->shmbuffer);
	} else {
		boxes = pixman_region32_rectangles(&glcontent->damage, &n);

		wl_shm_buffer_begin_access(buffer->shmbuffer);

		for (i = 0; i < n; i++) {
			pixman_box32_t box = nemocontent_transform_to_buffer_rect(&canvas->base, boxes[i]);

			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, box.x1);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, box.y1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, box.x1, box.y1,
					box.x2 - box.x1, box.y2 - box.y1,
					glcontent->format, glcontent->pixeltype, data);
		}

		wl_shm_buffer_end_access(buffer->shmbuffer);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

out:
	pixman_region32_fini(&glcontent->damage);
	pixman_region32_init(&glcontent->damage);
	glcontent->needs_full_upload = 0;

	nemobuffer_reference(&glcontent->buffer_reference, NULL);
}

int glrenderer_read_canvas(struct nemorenderer *base, struct nemocanvas *canvas, pixman_format_code_t format, void *pixels)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(&canvas->base, base->node);
	GLenum glformat;
	GLuint fbo;
	GLuint pbo;
	void *ptr;

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

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glcontent->textures[0], 0);

	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, glcontent->pitch * glcontent->height * 4, NULL, GL_STREAM_DRAW);

	glReadPixels(0, 0, glcontent->pitch, glcontent->height, glformat, GL_UNSIGNED_BYTE, NULL);

	ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, glcontent->pitch * glcontent->height * 4, GL_MAP_READ_BIT);
	if (ptr != NULL)
		memcpy(pixels, ptr, glcontent->pitch * glcontent->height * 4);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glDeleteBuffers(1, &pbo);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);

	return 0;
}

void *glrenderer_get_canvas_buffer(struct nemorenderer *base, struct nemocanvas *canvas)
{
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(&canvas->base, base->node);

	if (glcontent == NULL)
		return NULL;

	return glcontent->buffer_reference.buffer;
}

uint32_t nemocanvas_get_opengl_texture(struct nemocanvas *canvas, int index)
{
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context_on(&canvas->base, 0);

	if (glcontent == NULL)
		return 0;

	return glcontent->textures[index];
}
