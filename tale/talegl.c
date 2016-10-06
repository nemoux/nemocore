#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemotale.h>
#include <talenode.h>
#include <talegl.h>
#include <talepixman.h>
#include <glhelper.h>
#include <fbohelper.h>
#include <cliphelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#include <EGL/eglext.h>

struct nemogltale {
	struct nemomatrix transform;
	struct nemomatrix projection;

	struct glshader texture_shader_rgba;
	struct glshader texture_shader_rgbx;
	struct glshader texture_shader_egl_external;
	struct glshader texture_shader_y_uv;
	struct glshader texture_shader_y_u_v;
	struct glshader texture_shader_y_xuxv;
	struct glshader invert_color_shader;
	struct glshader solid_shader;

	struct glshader *current_shader;
};

struct taleegl {
	EGLDisplay display;
	EGLContext context;
	EGLConfig config;
	EGLSurface surface;

	pixman_region32_t damages[NEMOTALE_BUFFER_AGE_COUNT];

	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
	PFNEGLCREATEIMAGEKHRPROC create_image;
	PFNEGLDESTROYIMAGEKHRPROC destroy_image;
	PFNEGLSWAPBUFFERSWITHDAMAGEEXTPROC swap_buffers_with_damage;
	PFNEGLBINDWAYLANDDISPLAYWL bind_display;
	PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
	PFNEGLQUERYWAYLANDBUFFERWL query_buffer;

	int has_bind_display;
	int has_configless_context;
	int has_egl_image_external;
};

struct talefbo {
	GLuint fbo, dbo;
	GLuint texture;
};

static void nemotale_node_handle_destroy_signal(struct nemolistener *listener, void *data)
{
	struct taleglnode *context = (struct taleglnode *)container_of(listener, struct taleglnode, destroy_listener);

	nemolist_remove(&context->destroy_listener.link);

	if (context->has_texture_external == 0)
		glDeleteTextures(1, &context->texture);

#ifdef NEMOUX_WITH_OPENGL_PBO
	glDeleteBuffers(1, &context->pbo);
#endif

	free(context);
}

struct talenode *nemotale_node_create_gl(int32_t width, int32_t height)
{
	struct talenode *node;
	struct taleglnode *gcontext;

	node = (struct talenode *)malloc(sizeof(struct talenode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct talenode));

	node->glcontext = gcontext = (struct taleglnode *)malloc(sizeof(struct taleglnode));
	if (gcontext == NULL)
		goto err1;
	memset(gcontext, 0, sizeof(struct taleglnode));

	glGenTextures(1, &gcontext->texture);
	glBindTexture(GL_TEXTURE_2D, gcontext->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

#ifdef NEMOUX_WITH_OPENGL_PBO
	glGenBuffers(1, &gcontext->pbo);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gcontext->pbo);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	gcontext->pwidth = width;
	gcontext->pheight = height;
#endif

	nemotale_node_prepare(node);

	node->dirty = 0x2;
	node->geometry.width = width;
	node->geometry.height = height;

#ifdef NEMOUX_WITH_OPENGL_PBO
	node->dispatch_flush = nemotale_node_flush_gl_pbo;
	node->dispatch_map = nemotale_node_map_pbo;
	node->dispatch_unmap = nemotale_node_unmap_pbo;
#elif NEMOUX_WITH_OPENGL_UNPACK_SUBIMAGE
	node->dispatch_flush = nemotale_node_flush_gl_subimage;
#else
	node->dispatch_flush = nemotale_node_flush_gl;
#endif

	node->dispatch_resize = nemotale_node_resize_gl;

	pixman_region32_init_rect(&node->blend, 0, 0, width, height);
	pixman_region32_init_rect(&node->region, 0, 0, width, height);

	gcontext->destroy_listener.notify = nemotale_node_handle_destroy_signal;
	nemosignal_add(&node->destroy_signal, &gcontext->destroy_listener);

	return node;

err1:
	free(node);

	return NULL;
}

int nemotale_node_prepare_gl(struct talenode *node)
{
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	if (gcontext == NULL) {
		gcontext = (struct taleglnode *)malloc(sizeof(struct taleglnode));
		if (gcontext == NULL)
			return -1;
		memset(gcontext, 0, sizeof(struct taleglnode));

		glGenTextures(1, &gcontext->texture);
		glBindTexture(GL_TEXTURE_2D, gcontext->texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

#ifdef NEMOUX_WITH_OPENGL_PBO
		glGenBuffers(1, &gcontext->pbo);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gcontext->pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, node->geometry.width * node->geometry.height * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		gcontext->pwidth = node->geometry.width;
		gcontext->pheight = node->geometry.height;
#endif

#ifdef NEMOUX_WITH_OPENGL_PBO
		node->dispatch_flush = nemotale_node_flush_gl_pbo;
#elif NEMOUX_WITH_OPENGL_UNPACK_SUBIMAGE
		node->dispatch_flush = nemotale_node_flush_gl_subimage;
#else
		node->dispatch_flush = nemotale_node_flush_gl;
#endif

		node->glcontext = gcontext;

		gcontext->destroy_listener.notify = nemotale_node_handle_destroy_signal;
		nemosignal_add(&node->destroy_signal, &gcontext->destroy_listener);
	}

	return 0;
}

int nemotale_node_resize_gl(struct talenode *node, int32_t width, int32_t height)
{
	if (node->geometry.width != width || node->geometry.height != height) {
		struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

		node->dirty = 0x2;
		node->geometry.width = width;
		node->geometry.height = height;
		node->transform.dirty = 1;

		pixman_region32_init_rect(&node->blend, 0, 0, width, height);
		pixman_region32_init_rect(&node->region, 0, 0, width, height);

		if (gcontext->has_texture_external == 0) {
			glBindTexture(GL_TEXTURE_2D, gcontext->texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

#ifdef NEMOUX_WITH_OPENGL_PBO
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gcontext->pbo);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		gcontext->pwidth = width;
		gcontext->pheight = height;
#endif
	}

	return 0;
}

static inline void nemotale_use_shader(struct nemotale *tale, struct talenode *node, struct glshader *shader)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;

	if (!shader->program) {
		if (glshader_prepare(shader, shader->vertex_source, shader->fragment_source, 0) < 0) {
			nemolog_error("TALEGL", "failed to compile shader\n");
		}
	}

	if (context->current_shader == shader) {
		glUniform1f(shader->ualpha, node->alpha);
	} else {
		glUseProgram(shader->program);

		glUniformMatrix4fv(shader->uprojection, 1, GL_FALSE, context->projection.d);
		glUniform1i(shader->utextures[0], 0);
		glUniform1f(shader->ualpha, node->alpha);

		context->current_shader = shader;
	}
}

static int nemotale_calculate_edges(struct talenode *node, pixman_box32_t *rect, pixman_box32_t *rrect, GLfloat *ex, GLfloat *ey)
{
	struct clipcontext ctx;
	struct polygon8 surf = {
		{ rrect->x1, rrect->x2, rrect->x2, rrect->x1 },
		{ rrect->y1, rrect->y1, rrect->y2, rrect->y2 },
		4
	};
	GLfloat min_x, max_x, min_y, max_y;
	int i, n;

	ctx.clip.x1 = rect->x1;
	ctx.clip.y1 = rect->y1;
	ctx.clip.x2 = rect->x2;
	ctx.clip.y2 = rect->y2;

	for (i = 0; i < surf.n; i++) {
		nemotale_node_transform_to_global(node, surf.x[i], surf.y[i], &surf.x[i], &surf.y[i]);
	}

	min_x = max_x = surf.x[0];
	min_y = max_y = surf.y[0];

	for (i = 1; i < surf.n; i++) {
		min_x = MIN(min_x, surf.x[i]);
		max_x = MAX(max_x, surf.x[i]);
		min_y = MIN(min_y, surf.y[i]);
		max_y = MAX(max_y, surf.y[i]);
	}

	if ((min_x >= ctx.clip.x2) || (max_x <= ctx.clip.x1) ||
			(min_y >= ctx.clip.y2) || (max_y <= ctx.clip.y1))
		return 0;

	if (!node->transform.enable)
		return clip_simple(&ctx, &surf, ex, ey);

	n = clip_transformed(&ctx, &surf, ex, ey);
	if (n < 3)
		return 0;

	return n;
}

static int nemotale_repaint_region(struct nemotale *tale, struct talenode *node, pixman_region32_t *repaint, pixman_region32_t *region)
{
	pixman_box32_t *rects, *rrects, *rect, *rrect;
	GLfloat *vertices, *v, inv_width, inv_height;
	uint32_t *nvertices, nvtx = 0;
	int nrects, nrrects, i, j, k, n, s;

	rects = pixman_region32_rectangles(repaint, &nrects);
	rrects = pixman_region32_rectangles(region, &nrrects);

	vertices = (GLfloat *)malloc(nrects * nrrects * 8 * 4 * sizeof(GLfloat));
	nvertices = (uint32_t *)malloc(nrects * nrrects * sizeof(uint32_t));

	inv_width = 1.0f / node->geometry.width;
	inv_height = 1.0f / node->geometry.height;

	v = vertices;

	for (i = 0; i < nrects; i++) {
		rect = &rects[i];

		for (j = 0; j < nrrects; j++) {
			GLfloat sx, sy, bx, by;
			GLfloat ex[8], ey[8];

			rrect = &rrects[j];

			n = nemotale_calculate_edges(node, rect, rrect, ex, ey);
			if (n < 3)
				continue;

			for (k = 0; k < n; k++) {
				nemotale_node_transform_from_global(node, ex[k], ey[k], &sx, &sy);
				*(v++) = ex[k];
				*(v++) = ey[k];
				*(v++) = sx * inv_width;
				*(v++) = sy * inv_height;
			}

			nvertices[nvtx++] = n;
		}
	}

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[0]);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &vertices[2]);
	glEnableVertexAttribArray(1);

	for (i = 0, s = 0; i < nvtx; i++) {
		glDrawArrays(GL_TRIANGLE_FAN, s, nvertices[i]);

		s += nvertices[i];
	}

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	free(vertices);
	free(nvertices);
}

static void nemotale_repaint_node(struct nemotale *tale, struct talenode *node, pixman_region32_t *damage)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;
	pixman_region32_t repaint;
	pixman_region32_t blend;

	pixman_region32_init(&repaint);
	pixman_region32_intersect(&repaint, &node->boundingbox, damage);

	if (pixman_region32_not_empty(&repaint)) {
		struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

		glBindTexture(GL_TEXTURE_2D, gcontext->texture);

		if (node->has_smooth != 0 && node->transform.enable != 0) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		pixman_region32_init_rect(&blend, 0, 0, node->geometry.width, node->geometry.height);
		pixman_region32_subtract(&blend, &blend, &node->opaque);

		if (pixman_region32_not_empty(&node->opaque)) {
			nemotale_use_shader(tale, node, &context->texture_shader_rgba);

			glDisable(GL_BLEND);

			nemotale_repaint_region(tale, node, &repaint, &node->opaque);
		}

		if (pixman_region32_not_empty(&blend)) {
			nemotale_use_shader(tale, node, &context->texture_shader_rgba);

			glEnable(GL_BLEND);

			nemotale_repaint_region(tale, node, &repaint, &blend);
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		pixman_region32_fini(&blend);
	}

	pixman_region32_fini(&repaint);
}

static void nemotale_flush_node(struct nemotale *tale, struct talenode *node)
{
	if (node->pmcontext != NULL && node->needs_flush != 0)
		nemotale_node_flush(node);
}

struct nemotale *nemotale_create_gl(void)
{
	struct nemotale *tale;
	struct nemogltale *context;

	tale = (struct nemotale *)malloc(sizeof(struct nemotale));
	if (tale == NULL)
		return NULL;
	memset(tale, 0, sizeof(struct nemotale));

	nemotale_prepare(tale);

	tale->glcontext = context = (struct nemogltale *)malloc(sizeof(struct nemogltale));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct nemogltale));

	context->texture_shader_rgba.vertex_source = GLHELPER_VERTEX_SHADER;
	context->texture_shader_rgba.fragment_source = GLHELPER_TEXTURE_FRAGMENT_SHADER_RGBA;
	context->texture_shader_rgbx.vertex_source = GLHELPER_VERTEX_SHADER;
	context->texture_shader_rgbx.fragment_source = GLHELPER_TEXTURE_FRAGMENT_SHADER_RGBX;
	context->texture_shader_egl_external.vertex_source = GLHELPER_VERTEX_SHADER;
	context->texture_shader_egl_external.fragment_source = GLHELPER_TEXTURE_FRAGMENT_SHADER_EGL_EXTERNAL;
	context->texture_shader_y_uv.vertex_source = GLHELPER_VERTEX_SHADER;
	context->texture_shader_y_uv.fragment_source = GLHELPER_TEXTURE_FRAGMENT_SHADER_Y_UV;
	context->texture_shader_y_u_v.vertex_source = GLHELPER_VERTEX_SHADER;
	context->texture_shader_y_u_v.fragment_source = GLHELPER_TEXTURE_FRAGMENT_SHADER_Y_U_V;
	context->texture_shader_y_xuxv.vertex_source = GLHELPER_VERTEX_SHADER;
	context->texture_shader_y_xuxv.fragment_source = GLHELPER_TEXTURE_FRAGMENT_SHADER_Y_XUXV;
	context->solid_shader.vertex_source = GLHELPER_VERTEX_SHADER;
	context->solid_shader.fragment_source = GLHELPER_SOLID_FRAGMENT_SHADER;

	return tale;

err1:
	free(tale);

	return NULL;
}

void nemotale_destroy_gl(struct nemotale *tale)
{
	nemotale_finish(tale);

	free(tale->glcontext);
	free(tale);
}

int nemotale_make_current(struct nemotale *tale)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;
	struct taleegl *egl = (struct taleegl *)tale->backend;

	if (!eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context))
		return -1;

	return 0;
}

static inline void nemotale_composite_ready(struct nemotale *tale, int flip)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;

	if (tale->transform.dirty != 0) {
		nemomatrix_init_identity(&context->transform);
		nemomatrix_multiply(&context->transform, &tale->transform.matrix);

		context->projection = context->transform;
		nemomatrix_translate(&context->projection, -tale->width / 2.0f, -tale->height / 2.0f);
		nemomatrix_scale(&context->projection, 2.0f / tale->width, 2.0f / tale->height * flip);

		tale->transform.dirty = 0;
	}

	context->current_shader = NULL;
}

struct taleegl *nemotale_create_egl(EGLDisplay egl_display, EGLContext egl_context, EGLConfig egl_config, EGLNativeWindowType egl_window)
{
	struct taleegl *egl;
	const char *extensions;
	struct nemotale *tale;
	struct nemoegltale *context;
	int i;

	egl = (struct taleegl *)malloc(sizeof(struct taleegl));
	if (egl == NULL)
		return NULL;
	memset(egl, 0, sizeof(struct taleegl));

	egl->display = egl_display;
	egl->context = egl_context;
	egl->config = egl_config;

	if (!eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl->context))
		goto err1;

	extensions = (const char *)eglQueryString(egl->display, EGL_EXTENSIONS);
	if (extensions == NULL)
		goto err1;

	if (strstr(extensions, "EGL_WL_bind_wayland_display"))
		egl->has_bind_display = 1;

	if (strstr(extensions, "EGL_EXT_swap_buffers_with_damage") && strstr(extensions, "EGL_EXT_buffer_age"))
		egl->swap_buffers_with_damage = (void *)eglGetProcAddress("eglSwapBuffersWithDamageEXT");

	if (strstr(extensions, "EGL_MESA_configless_context"))
		egl->has_configless_context = 1;

	if (strstr(extensions, "GL_OES_EGL_image_external"))
		egl->has_egl_image_external = 1;

	egl->create_image = (void *)eglGetProcAddress("eglCreateImageKHR");
	egl->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");
	egl->bind_display = (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
	egl->unbind_display = (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");
	egl->query_buffer = (void *)eglGetProcAddress("eglQueryWaylandBufferWL");
	egl->image_target_texture_2d = (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

	egl->surface = eglCreateWindowSurface(egl->display, egl->config, egl_window, NULL);
	if (egl->surface == EGL_NO_SURFACE)
		goto err1;

	for (i = 0; i < NEMOTALE_BUFFER_AGE_COUNT; i++) {
		pixman_region32_init(&egl->damages[i]);
	}

	return egl;

err1:
	free(egl);

	return NULL;
}

void nemotale_destroy_egl(struct taleegl *egl)
{
	int i;

	for (i = 0; i < NEMOTALE_BUFFER_AGE_COUNT; i++) {
		pixman_region32_fini(&egl->damages[i]);
	}

	eglMakeCurrent(egl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	eglDestroySurface(egl->display, egl->surface);

	free(egl);
}

static inline void nemotale_fetch_buffer_damage(struct taleegl *egl, pixman_region32_t *region, pixman_region32_t *damage)
{
	EGLint buffer_age = 0;
	int i;

	if (eglQuerySurface(egl->display, egl->surface, EGL_BUFFER_AGE_EXT, &buffer_age) == EGL_FALSE)
		nemolog_error("TALEGL", "failed to query buffer age\n");

	if (buffer_age == 0 || buffer_age - 1 > NEMOTALE_BUFFER_AGE_COUNT) {
		pixman_region32_copy(damage, region);
	} else {
		for (i = 0; i < buffer_age - 1; i++) {
			pixman_region32_union(damage, damage, &egl->damages[i]);
		}
	}
}

static inline void nemotale_rotate_buffer_damage(struct taleegl *egl, pixman_region32_t *damage)
{
	int i;

	for (i = NEMOTALE_BUFFER_AGE_COUNT - 1; i >= 1; i--) {
		pixman_region32_copy(&egl->damages[i], &egl->damages[i - 1]);
	}

	pixman_region32_copy(&egl->damages[0], damage);
}

static inline int nemotale_composite_egl_in(struct nemotale *tale)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;
	struct taleegl *egl = (struct taleegl *)tale->backend;
	struct talenode *node;
	EGLBoolean r;

	if (!eglMakeCurrent(egl->display, egl->surface, egl->surface, egl->context))
		return -1;

	nemotale_composite_ready(tale, -1);

	glViewport(0, 0, tale->width, tale->height);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	if (egl->swap_buffers_with_damage != NULL) {
		pixman_region32_t buffer_damage, total_damage;
		pixman_box32_t *rects;
		EGLint *edamages, *edamage;
		int nrects;
		int i;

		pixman_region32_init(&total_damage);
		pixman_region32_init(&buffer_damage);

		nemotale_fetch_buffer_damage(egl, &tale->region, &buffer_damage);
		nemotale_rotate_buffer_damage(egl, &tale->damage);

		pixman_region32_union(&total_damage, &buffer_damage, &tale->damage);

		nemolist_for_each(node, &tale->node_list, link) {
			nemotale_flush_node(tale, node);
		}

		nemolist_for_each(node, &tale->node_list, link) {
			nemotale_repaint_node(tale, node, &total_damage);
		}

		pixman_region32_fini(&total_damage);
		pixman_region32_fini(&buffer_damage);

		pixman_region32_init(&buffer_damage);
		pixman_region32_union(&buffer_damage, &buffer_damage, &tale->damage);

		rects = pixman_region32_rectangles(&buffer_damage, &nrects);

		edamages = (EGLint *)malloc(sizeof(EGLint) * nrects * 4);
		if (edamages == NULL)
			return -1;

		for (i = 0, edamage = edamages; i < nrects; i++) {
			struct nemovector v1 = { { rects[i].x1, rects[i].y1, 0.0f, 1.0f } };
			struct nemovector v2 = { { rects[i].x2, rects[i].y2, 0.0f, 1.0f } };

			nemomatrix_transform(&context->transform, &v1);
			nemomatrix_transform(&context->transform, &v2);

			rects[i].x1 = v1.f[0] / v1.f[3];
			rects[i].y1 = v1.f[1] / v1.f[3];
			rects[i].x2 = v2.f[0] / v2.f[3];
			rects[i].y2 = v2.f[1] / v2.f[3];

			*edamage++ = MAX(rects[i].x1 - 1, 0);
			*edamage++ = MAX(tale->height - rects[i].y2 - 1, 0);
			*edamage++ = MIN(rects[i].x2 - rects[i].x1 + 1, tale->width);
			*edamage++ = MIN(rects[i].y2 - rects[i].y1 + 1, tale->height);
		}

		r = egl->swap_buffers_with_damage(egl->display, egl->surface, edamages, nrects);

		free(edamages);
		pixman_region32_fini(&buffer_damage);
	} else {
		nemolist_for_each(node, &tale->node_list, link) {
			nemotale_flush_node(tale, node);
		}

		nemolist_for_each(node, &tale->node_list, link) {
			nemotale_repaint_node(tale, node, &tale->damage);
		}

		r = eglSwapBuffers(egl->display, egl->surface);
	}

	return r == EGL_TRUE;
}

int nemotale_composite_egl(struct nemotale *tale, pixman_region32_t *region)
{
	int r;

	nemotale_update_node(tale);
	nemotale_accumulate_damage(tale);

	r = nemotale_composite_egl_in(tale);

	if (region != NULL)
		pixman_region32_union(region, region, &tale->damage);

	nemotale_flush_damage(tale);

	return r;
}

int nemotale_composite_egl_full(struct nemotale *tale)
{
	int r;

	nemotale_update_node(tale);
	nemotale_damage_all(tale);

	r = nemotale_composite_egl_in(tale);

	return r;
}

struct talefbo *nemotale_create_fbo(GLuint texture, int32_t width, int32_t height)
{
	struct talefbo *fbo;

	fbo = (struct talefbo *)malloc(sizeof(struct talefbo));
	if (fbo == NULL)
		return NULL;
	memset(fbo, 0, sizeof(struct talefbo));

	fbo->texture = texture;

	if (fbo_prepare_context(fbo->texture, width, height, &fbo->fbo, &fbo->dbo) < 0)
		goto err1;

	return fbo;

err1:
	free(fbo);

	return NULL;
}

void nemotale_destroy_fbo(struct talefbo *fbo)
{
	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteRenderbuffers(1, &fbo->dbo);

	free(fbo);
}

int nemotale_resize_fbo(struct talefbo *fbo, int32_t width, int32_t height)
{
	glDeleteFramebuffers(1, &fbo->fbo);
	glDeleteRenderbuffers(1, &fbo->dbo);

	return fbo_prepare_context(fbo->texture, width, height, &fbo->fbo, &fbo->dbo);
}

int nemotale_composite_fbo(struct nemotale *tale, pixman_region32_t *region)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;
	struct talefbo *fbo = (struct talefbo *)tale->backend;
	struct talenode *node;
	int i;

	nemotale_update_node(tale);
	nemotale_accumulate_damage(tale);

	nemotale_composite_ready(tale, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

	glViewport(0, 0, tale->width, tale->height);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	nemolist_for_each(node, &tale->node_list, link) {
		nemotale_flush_node(tale, node);
	}

	nemolist_for_each(node, &tale->node_list, link) {
		nemotale_repaint_node(tale, node, &tale->damage);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	if (region != NULL)
		pixman_region32_union(region, region, &tale->damage);

	nemotale_flush_damage(tale);

	return 0;
}

int nemotale_composite_fbo_full(struct nemotale *tale)
{
	struct nemogltale *context = (struct nemogltale *)tale->glcontext;
	struct talefbo *fbo = (struct talefbo *)tale->backend;
	struct talenode *node;

	nemotale_update_node(tale);
	nemotale_damage_all(tale);

	nemotale_composite_ready(tale, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo->fbo);

	glViewport(0, 0, tale->width, tale->height);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	nemolist_for_each(node, &tale->node_list, link) {
		nemotale_flush_node(tale, node);
	}

	nemolist_for_each(node, &tale->node_list, link) {
		nemotale_repaint_node(tale, node, &tale->damage);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return 0;
}

int nemotale_node_flush_gl(struct talenode *node)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	if (pcontext != NULL && node->needs_flush != 0) {
		glBindTexture(GL_TEXTURE_2D, gcontext->texture);

		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
				node->geometry.width, node->geometry.height, 0,
				GL_BGRA_EXT, GL_UNSIGNED_BYTE, pcontext->data);

		glBindTexture(GL_TEXTURE_2D, 0);

		node->needs_flush = 0;
		node->needs_full_upload = 0;
	}

	return 0;
}

int nemotale_node_flush_gl_pbo(struct talenode *node)
{
#ifdef NEMOUX_WITH_OPENGL_PBO
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	if (pcontext != NULL && node->needs_flush != 0) {
		GLubyte *ptr;

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gcontext->pbo);

		if (gcontext->pwidth != node->geometry.width || gcontext->pheight != node->geometry.height) {
			glBufferData(GL_PIXEL_UNPACK_BUFFER, node->geometry.width * node->geometry.height * 4, NULL, GL_DYNAMIC_DRAW);

			gcontext->pwidth = node->geometry.width;
			gcontext->pheight = node->geometry.height;

			node->needs_full_upload = 1;
		}

		ptr = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, node->geometry.width * node->geometry.height * 4, GL_MAP_WRITE_BIT);

		if (node->needs_full_upload != 0) {
			memcpy(ptr, pcontext->data, node->geometry.width * node->geometry.height * 4);

			node->needs_full_upload = 0;
		} else {
			pixman_box32_t *rects;
			int i, n, l;

			rects = pixman_region32_rectangles(&node->damage, &n);

			for (i = 0; i < n; i++) {
				pixman_box32_t box = rects[i];

				box.x1 = MIN(MAX(box.x1, 0), node->geometry.width);
				box.y1 = MIN(MAX(box.y1, 0), node->geometry.height);
				box.x2 = MAX(MIN(box.x2, node->geometry.width), 0);
				box.y2 = MAX(MIN(box.y2, node->geometry.height), 0);

				for (l = box.y1; l < box.y2; l++) {
					uint32_t offset = (l * node->geometry.width + box.x1) * 4;
					uint32_t size = (box.x2 - box.x1) * 4;

					memcpy(ptr + offset, pcontext->data + offset, size);
				}
			}
		}

		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

		glBindTexture(GL_TEXTURE_2D, gcontext->texture);

		glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, node->geometry.width);

		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
				node->geometry.width, node->geometry.height, 0,
				GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);

		glBindTexture(GL_TEXTURE_2D, 0);

		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		node->needs_flush = 0;
	}
#endif

	return 0;
}

void *nemotale_node_map_pbo(struct talenode *node)
{
#ifdef NEMOUX_WITH_OPENGL_PBO
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, gcontext->pbo);

	if (gcontext->pwidth != node->geometry.width || gcontext->pheight != node->geometry.height) {
		glBufferData(GL_PIXEL_UNPACK_BUFFER, node->geometry.width * node->geometry.height * 4, NULL, GL_DYNAMIC_DRAW);

		gcontext->pwidth = node->geometry.width;
		gcontext->pheight = node->geometry.height;
	}

	return glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, node->geometry.width * node->geometry.height * 4, GL_MAP_WRITE_BIT);
#else
	return NULL;
#endif
}

int nemotale_node_unmap_pbo(struct talenode *node)
{
#ifdef NEMOUX_WITH_OPENGL_PBO
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

	glBindTexture(GL_TEXTURE_2D, gcontext->texture);

	glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, node->geometry.width);

	glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
			node->geometry.width, node->geometry.height, 0,
			GL_BGRA_EXT, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	node->needs_flush = 0;
	node->needs_full_upload = 0;
#endif

	return 0;
}

int nemotale_node_flush_gl_subimage(struct talenode *node)
{
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	if (pcontext != NULL && node->needs_flush != 0) {
		glBindTexture(GL_TEXTURE_2D, gcontext->texture);

		glPixelStorei(GL_UNPACK_ROW_LENGTH_EXT, node->geometry.width);

		if (node->needs_full_upload != 0) {
			glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
			glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT,
					node->geometry.width, node->geometry.height, 0,
					GL_BGRA_EXT, GL_UNSIGNED_BYTE, pcontext->data);

			node->needs_full_upload = 0;
		} else {
			pixman_box32_t *rects;
			int i, n;

			rects = pixman_region32_rectangles(&node->damage, &n);

			for (i = 0; i < n; i++) {
				pixman_box32_t box = rects[i];

				box.x1 = MIN(MAX(box.x1, 0), node->geometry.width);
				box.y1 = MIN(MAX(box.y1, 0), node->geometry.height);
				box.x2 = MAX(MIN(box.x2, node->geometry.width), 0);
				box.y2 = MAX(MIN(box.y2, node->geometry.height), 0);

				glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, box.x1);
				glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, box.y1);
				glTexSubImage2D(GL_TEXTURE_2D, 0, box.x1, box.y1,
						box.x2 - box.x1, box.y2 - box.y1,
						GL_BGRA_EXT, GL_UNSIGNED_BYTE, pcontext->data);
			}
		}

		glBindTexture(GL_TEXTURE_2D, 0);

		node->needs_flush = 0;
	}

	return 0;
}

int nemotale_node_flush_gl_external(struct talenode *node)
{
	return 0;
}

int nemotale_node_set_texture(struct talenode *node, GLuint texture)
{
	struct taleglnode *context = (struct taleglnode *)node->glcontext;

	if (context->has_texture_external == 0) {
		glDeleteTextures(1, &context->texture);

		context->has_texture_external = 1;

		node->dispatch_flush = nemotale_node_flush_gl_external;
	}

	context->texture = texture;

	return 0;
}

GLuint nemotale_node_get_texture(struct talenode *node)
{
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;

	return gcontext->texture;
}

int nemotale_node_use_pbo(struct talenode *node, int use_pbo)
{
#ifdef NEMOUX_WITH_OPENGL_PBO
	struct taleglnode *gcontext = (struct taleglnode *)node->glcontext;
	struct talepmnode *pcontext = (struct talepmnode *)node->pmcontext;

	if (pcontext == NULL || gcontext->has_texture_external != 0)
		return 0;

	if (use_pbo != 0) {
		node->dispatch_flush = nemotale_node_flush_gl_pbo;
	} else {
#ifdef NEMOUX_WITH_OPENGL_UNPACK_SUBIMAGE
		node->dispatch_flush = nemotale_node_flush_gl_subimage;
#else
		node->dispatch_flush = nemotale_node_flush_gl;
#endif
	}
#endif

	return 0;
}
