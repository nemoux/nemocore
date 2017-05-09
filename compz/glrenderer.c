#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <glprivate.h>

#include <glrenderer.h>
#include <glcanvas.h>
#include <renderer.h>
#include <compz.h>
#include <screen.h>
#include <canvas.h>
#include <view.h>
#include <drmnode.h>
#include <content.h>
#include <waylandhelper.h>
#include <cliphelper.h>
#include <nemomisc.h>
#include <nemolog.h>

static int glrenderer_read_pixels(struct nemorenderer *base, struct nemoscreen *screen, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glsurface *surface = (struct glsurface *)screen->gcontext;
	GLenum glformat;
	GLuint pbo;
	void *ptr;

	if (eglMakeCurrent(renderer->egl_display, surface->egl_surface, surface->egl_surface, renderer->egl_context) == EGL_FALSE) {
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

	glGenBuffers(1, &pbo);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
	glBufferData(GL_PIXEL_PACK_BUFFER, width * height * 4, NULL, GL_STATIC_DRAW);

	glReadPixels(x, y, width, height, glformat, GL_UNSIGNED_BYTE, NULL);

	ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, width * height * 4, GL_MAP_READ_BIT);
	if (ptr != NULL)
		memcpy(pixels, ptr, width * height * 4);
	glUnmapBuffer(GL_PIXEL_PACK_BUFFER);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glDeleteBuffers(1, &pbo);

	return 0;
}

static inline void glrenderer_fetch_buffer_damage(struct glrenderer *renderer, struct glsurface *surface, pixman_region32_t *region, pixman_region32_t *damage)
{
	EGLint buffer_age = 0;
	int i;

	if (eglQuerySurface(renderer->egl_display, surface->egl_surface, EGL_BUFFER_AGE_EXT, &buffer_age) == EGL_FALSE)
		nemolog_error("GLRENDERER", "failed to query buffer age\n");

	if (buffer_age == 0 || buffer_age - 1 > GLRENDERER_BUFFER_AGE_COUNT) {
		pixman_region32_copy(damage, region);
	} else {
		for (i = 0; i < buffer_age - 1; i++) {
			pixman_region32_union(damage, damage, &surface->damages[i]);
		}
	}
}

static inline void glrenderer_rotate_buffer_damage(struct glrenderer *renderer, struct glsurface *surface, pixman_region32_t *damage)
{
	int i;

	for (i = GLRENDERER_BUFFER_AGE_COUNT - 1; i >= 1; i--) {
		pixman_region32_copy(&surface->damages[i], &surface->damages[i - 1]);
	}

	pixman_region32_copy(&surface->damages[0], damage);
}

static inline void glrenderer_use_shader(struct glrenderer *renderer, struct glcompz *shader)
{
	if (!shader->program) {
		if (glcompz_prepare(shader, shader->vertex_source, shader->fragment_source) < 0) {
			nemolog_error("GLRENDERER", "failed to compile shader\n");
		}
	}

	if (renderer->current_shader == shader)
		return;

	glUseProgram(shader->program);

	renderer->current_shader = shader;
}

static inline void glrenderer_clear_shader(struct glrenderer *renderer)
{
	renderer->current_shader = NULL;
}

static inline void glrenderer_use_uniforms(struct glcompz *shader, struct nemoview *view, struct nemoscreen *screen)
{
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(view->content, screen->node);
	int i;

	glUniformMatrix4fv(shader->uprojection, 1, GL_FALSE, nemomatrix_get_array(&screen->render.matrix));
	glUniform4fv(shader->ucolor, 1, glcontent->colors);
	glUniform1f(shader->ualpha, view->alpha);

	for (i = 0; i < glcontent->ntextures; i++) {
		glUniform1i(shader->utextures[i], i);
	}
}

static int glrenderer_calculate_edges(struct nemoview *view, pixman_box32_t *rect, pixman_box32_t *rrect, GLfloat *ex, GLfloat *ey)
{
	struct clip clip;
	struct polygon8 poly = {
		{ rrect->x1, rrect->x2, rrect->x2, rrect->x1 },
		{ rrect->y1, rrect->y1, rrect->y2, rrect->y2 },
		4
	};
	GLfloat min_x, max_x, min_y, max_y;
	int i, n;

	clip_set_region(&clip, rect->x1, rect->y1, rect->x2, rect->y2);

	for (i = 0; i < poly.n; i++) {
		nemoview_transform_to_global_nocheck(view, poly.x[i], poly.y[i], &poly.x[i], &poly.y[i]);
	}

	min_x = max_x = poly.x[0];
	min_y = max_y = poly.y[0];

	for (i = 1; i < poly.n; i++) {
		min_x = MIN(min_x, poly.x[i]);
		max_x = MAX(max_x, poly.x[i]);
		min_y = MIN(min_y, poly.y[i]);
		max_y = MAX(max_y, poly.y[i]);
	}

	if (clip_check_minmax(&clip, min_x, min_y, max_x, max_y) != 0)
		return 0;

	if (!view->transform.enable)
		return clip_simple(&clip, &poly, ex, ey);

	n = clip_transformed(&clip, &poly, ex, ey);
	if (n < 3)
		return 0;

	return n;
}

static int glrenderer_texture_region(struct glrenderer *renderer, struct nemoview *view, pixman_region32_t *repaint, pixman_region32_t *region)
{
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(view->content, renderer->base.node);
	pixman_box32_t *rects, *rrects, *rect, *rrect;
	GLfloat *v, inv_width, inv_height;
	uint32_t *vtxcnt, nvtx = 0;
	int nrects, nrrects, i, j, k;

	rects = pixman_region32_rectangles(repaint, &nrects);
	rrects = pixman_region32_rectangles(region, &nrrects);

	v = wl_array_add(&renderer->vertices, nrects * nrrects * 8 * 4 * sizeof(GLfloat));
	vtxcnt = wl_array_add(&renderer->vtxcnt, nrects * nrrects * sizeof(uint32_t));

	inv_width = 1.0f / glcontent->pitch;
	inv_height = 1.0f / glcontent->height;

	for (i = 0; i < nrects; i++) {
		rect = &rects[i];

		for (j = 0; j < nrrects; j++) {
			GLfloat sx, sy, bx, by;
			GLfloat ex[8], ey[8];
			int n;

			rrect = &rrects[j];

			n = glrenderer_calculate_edges(view, rect, rrect, ex, ey);
			if (n < 3)
				continue;

			for (k = 0; k < n; k++) {
				nemoview_transform_from_global_nocheck(view, ex[k], ey[k], &sx, &sy);
				*(v++) = ex[k];
				*(v++) = ey[k];

				nemocontent_transform_to_buffer_point(view->content, sx, sy, &bx, &by);

				*(v++) = bx * inv_width;
				if (glcontent->y_inverted)
					*(v++) = by * inv_height;
				else
					*(v++) = (glcontent->height - by) * inv_height;
			}

			vtxcnt[nvtx++] = n;
		}
	}

	return nvtx;
}

static void glrenderer_draw_region(struct glrenderer *renderer, struct nemoview *view, pixman_region32_t *repaint, pixman_region32_t *region)
{
	GLfloat *v;
	uint32_t *vtxcnt;
	int i, first, nfans;

	nfans = glrenderer_texture_region(renderer, view, repaint, region);

	v = renderer->vertices.data;
	vtxcnt = renderer->vtxcnt.data;

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &v[0]);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), &v[2]);
	glEnableVertexAttribArray(1);

	for (i = 0, first = 0; i < nfans; i++) {
		glDrawArrays(GL_TRIANGLE_FAN, first, vtxcnt[i]);

		first += vtxcnt[i];
	}

	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	renderer->vertices.size = 0;
	renderer->vtxcnt.size = 0;
}

static void glrenderer_draw_view(struct glrenderer *renderer, struct nemoview *view, struct nemoscreen *screen, pixman_region32_t *damage)
{
	struct glcontent *glcontent = (struct glcontent *)nemocontent_get_opengl_context(view->content, renderer->base.node);
	pixman_region32_t repaint;
	pixman_region32_t blend;
	GLint filter = GL_NEAREST;
	int i;

	if (glcontent == NULL || glcontent->shader == NULL)
		return;

	pixman_region32_init(&repaint);
	pixman_region32_intersect(&repaint, &view->transform.boundingbox, damage);
	pixman_region32_subtract(&repaint, &repaint, &view->clip);

	if (!pixman_region32_not_empty(&repaint))
		goto out;

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glrenderer_use_shader(renderer, glcontent->shader);
	glrenderer_use_uniforms(glcontent->shader, view, screen);

	if ((nemoview_has_state(view, NEMOVIEW_SMOOTH_STATE) != 0) &&
			(view->transform.enable != 0 || screen->transform.enable != 0 || nemocontent_get_buffer_scale(view->content) != 1))
		filter = GL_LINEAR;

	for (i = 0; i < glcontent->ntextures; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(glcontent->target, glcontent->textures[i]);
		glTexParameteri(glcontent->target, GL_TEXTURE_MIN_FILTER, filter);
		glTexParameteri(glcontent->target, GL_TEXTURE_MAG_FILTER, filter);
	}

	pixman_region32_init_rect(&blend, 0, 0, view->content->width, view->content->height);
	pixman_region32_subtract(&blend, &blend, &view->content->opaque);

	if (pixman_region32_not_empty(&view->content->opaque)) {
		glrenderer_use_shader(renderer, &renderer->texture_shader_rgbx);
		glrenderer_use_uniforms(&renderer->texture_shader_rgbx, view, screen);

		if (view->alpha < 1.0f)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);

		glrenderer_draw_region(renderer, view, &repaint, &view->content->opaque);
	}

	if (pixman_region32_not_empty(&blend)) {
		glrenderer_use_shader(renderer, glcontent->shader);
		glrenderer_use_uniforms(glcontent->shader, view, screen);

		glEnable(GL_BLEND);

		glrenderer_draw_region(renderer, view, &repaint, &blend);
	}

	pixman_region32_fini(&blend);

	for (i = 0; i < glcontent->ntextures; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(glcontent->target, 0);
	}

out:
	pixman_region32_fini(&repaint);
}

static void glrenderer_repaint_views(struct glrenderer *renderer, struct nemoscreen *screen, pixman_region32_t *damage)
{
	struct nemocompz *compz = screen->compz;
	struct nemolayer *layer;
	struct nemoview *view, *child;

	glViewport(0, 0, screen->width, screen->height);

	glrenderer_clear_shader(renderer);

	wl_list_for_each_reverse(layer, &compz->layer_list, link) {
		wl_list_for_each_reverse(view, &layer->view_list, layer_link) {
			glrenderer_draw_view(renderer, view, screen, damage);

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each_reverse(child, &view->children_list, children_link) {
					glrenderer_draw_view(renderer, child, screen, damage);
				}
			}
		}
	}
}

static void glrenderer_repaint_screen(struct nemorenderer *base, struct nemoscreen *screen, pixman_region32_t *screen_damage)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glsurface *surface = (struct glsurface *)screen->gcontext;
	EGLBoolean r;

	if (eglMakeCurrent(renderer->egl_display, surface->egl_surface, surface->egl_surface, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}

	if (renderer->swap_buffers_with_damage != NULL) {
		pixman_region32_t buffer_damage, total_damage;
		pixman_box32_t *rects;
		EGLint *edamages, *edamage;
		int i, nrects, buffer_height;

		pixman_region32_init(&total_damage);
		pixman_region32_init(&buffer_damage);

		glrenderer_fetch_buffer_damage(renderer, surface, &screen->region, &buffer_damage);
		glrenderer_rotate_buffer_damage(renderer, surface, screen_damage);

		pixman_region32_union(&total_damage, &buffer_damage, screen_damage);

		glrenderer_repaint_views(renderer, screen, &total_damage);

		pixman_region32_fini(&total_damage);
		pixman_region32_fini(&buffer_damage);

		pixman_region32_init(&buffer_damage);

		wayland_transform_region(screen->width, screen->height, WL_OUTPUT_TRANSFORM_NORMAL, 1, screen_damage, &buffer_damage);

		rects = pixman_region32_rectangles(&buffer_damage, &nrects);

		edamages = (EGLint *)malloc(sizeof(EGLint) * nrects * 4);
		if (edamages == NULL) {
			nemolog_error("GLRENDERER", "failed to allocate egl damage array\n");
			return;
		}

		buffer_height = screen->current_mode->height;

		edamage = edamages;

		for (i = 0; i < nrects; i++) {
			*edamage++ = rects[i].x1;
			*edamage++ = buffer_height - rects[i].y2;
			*edamage++ = rects[i].x2 - rects[i].x1;
			*edamage++ = rects[i].y2 - rects[i].y1;
		}

		r = renderer->swap_buffers_with_damage(renderer->egl_display, surface->egl_surface, edamages, nrects);

		free(edamages);
		pixman_region32_fini(&buffer_damage);
	} else {
		glrenderer_repaint_views(renderer, screen, screen_damage);

		r = eglSwapBuffers(renderer->egl_display, surface->egl_surface);
	}

	if (r == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to swap egl buffers\n");
		return;
	}
}

static int glrenderer_choose_config(struct glrenderer *renderer, const EGLint *attribs, const EGLint *visualid, EGLConfig *config)
{
	EGLint count = 0;
	EGLint matched = 0;
	EGLint id;
	EGLConfig *configs;
	int i;

	if (!eglGetConfigs(renderer->egl_display, NULL, 0, &count) || count < 1)
		return -1;

	configs = (EGLConfig *)malloc(sizeof(EGLConfig) * count);
	if (configs == NULL)
		return -1;
	memset(configs, 0, sizeof(EGLConfig) * count);

	if (!eglChooseConfig(renderer->egl_display, attribs, configs, count, &matched))
		goto err1;

	for (i = 0; i < matched; i++) {
		if (visualid != NULL) {
			if (!eglGetConfigAttrib(renderer->egl_display, configs[i], EGL_NATIVE_VISUAL_ID, &id))
				continue;

			if (id != 0 && id != *visualid)
				continue;
		}

		*config = configs[i];

		free(configs);

		return 0;
	}


err1:
	free(configs);

	return -1;
}

static int glrenderer_prepare_egl_extentsion(struct glrenderer *renderer)
{
	const char *extensions;

	renderer->create_image = (void *)eglGetProcAddress("eglCreateImageKHR");
	renderer->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");
	renderer->bind_display = (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
	renderer->unbind_display = (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");
	renderer->query_buffer = (void *)eglGetProcAddress("eglQueryWaylandBufferWL");
	renderer->image_target_texture_2d = (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");

	extensions = (const char *)eglQueryString(renderer->egl_display, EGL_EXTENSIONS);
	if (extensions == NULL) {
		nemolog_error("GLRENDERER", "failed to retrieve egl extension string\n");
		return -1;
	}

	if (strstr(extensions, "EGL_WL_bind_wayland_display")) {
		renderer->has_bind_display = 1;
	}

	if (strstr(extensions, "EGL_EXT_swap_buffers_with_damage") && strstr(extensions, "EGL_EXT_buffer_age"))
		renderer->swap_buffers_with_damage = (void *)eglGetProcAddress("eglSwapBuffersWithDamageEXT");
	else
		nemolog_warning("GLRENDERER", "no egl swap buffers with damage extension\n");

	if (strstr(extensions, "EGL_MESA_configless_context"))
		renderer->has_configless_context = 1;
	else
		nemolog_warning("GLRENDERER", "no egl configless context extension\n");

	if (strstr(extensions, "EGL_KHR_surfaceless_context"))
		renderer->has_surfaceless_context = 1;
	else
		nemolog_warning("GLRENDERER", "no egl surfaceless context extension\n");

	if (strstr(extensions, "EGL_EXT_image_dma_buf_import"))
		renderer->has_dmabuf_import = 1;
	else
		nemolog_warning("GLRENDERER", "no egl dmabuf import extension\n");

	return 0;
}

struct nemorenderer *glrenderer_create(struct rendernode *node, EGLNativeDisplayType display, const EGLint *visualid)
{
	static EGLint attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SAMPLE_BUFFERS, 1,
		EGL_SAMPLES, 4,
		EGL_NONE
	};

	struct nemocompz *compz = node->compz;
	struct glrenderer *renderer;
	EGLint major, minor;

	renderer = (struct glrenderer *)malloc(sizeof(struct glrenderer));
	if (renderer == NULL)
		return NULL;
	memset(renderer, 0, sizeof(struct glrenderer));

	renderer->base.node = node;
	renderer->base.read_pixels = glrenderer_read_pixels;
	renderer->base.repaint_screen = glrenderer_repaint_screen;
	renderer->base.prepare_buffer = glrenderer_prepare_buffer;
	renderer->base.attach_canvas = glrenderer_attach_canvas;
	renderer->base.flush_canvas = glrenderer_flush_canvas;
	renderer->base.read_canvas = glrenderer_read_canvas;
	renderer->base.get_canvas_buffer = glrenderer_get_canvas_buffer;
	renderer->base.destroy = glrenderer_destroy;
	renderer->base.make_current = glrenderer_make_current;

	renderer->attribs = attribs;

	renderer->egl_display = eglGetDisplay(display);
	if (renderer->egl_display == EGL_NO_DISPLAY) {
		nemolog_error("GLRENDERER", "failed to create egl display\n");
		goto err1;
	}

	if (!eglInitialize(renderer->egl_display, &major, &minor)) {
		nemolog_error("GLRENDERER", "failed to initialize egl display\n");
		goto err1;
	}

	if (glrenderer_choose_config(renderer, renderer->attribs, visualid, &renderer->egl_config) < 0) {
		nemolog_error("GLRENDERER", "failed to choose egl config\n");
		goto err1;
	}

	if (glrenderer_prepare_egl_extentsion(renderer) < 0) {
		nemolog_error("GLRENDERER", "failed to prepare egl extensions\n");
		goto err1;
	}

	if (compz->renderer == NULL) {
		if (renderer->has_bind_display != 0) {
			renderer->bind_display(renderer->egl_display, compz->display);
		}

		compz->renderer = &renderer->base;
	}

	wl_display_add_shm_format(compz->display, WL_SHM_FORMAT_RGB565);

	wl_signal_init(&renderer->destroy_signal);

	wl_array_init(&renderer->vertices);
	wl_array_init(&renderer->vtxcnt);

	return &renderer->base;

err1:
	free(renderer);

	return NULL;
}

void glrenderer_destroy(struct nemorenderer *base)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);

	wl_signal_emit(&renderer->destroy_signal, renderer);

	if (renderer->has_bind_display != 0) {
		renderer->unbind_display(renderer->egl_display, base->node->compz->display);
	}

	eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

#if	0
	eglTerminate(renderer->egl_display);
	eglReleaseThread();
#endif

	wl_array_release(&renderer->vertices);
	wl_array_release(&renderer->vtxcnt);

	free(renderer);
}

void glrenderer_make_current(struct nemorenderer *base)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);

	if (eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context) == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return;
	}
}

static int glrenderer_prepare_egl_context(struct glrenderer *renderer, struct nemoscreen *screen, EGLSurface egl_surface)
{
	static EGLint attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};
	EGLConfig egl_config;
	EGLBoolean r;
	const char *vendor;
	const char *version;
	const char *extensions;

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		nemolog_error("GLRENDERER", "failed to bind EGL_OPENGL_ES_API\n");
		return -1;
	}

	egl_config = renderer->egl_config;

	if (renderer->has_configless_context)
		egl_config = EGL_NO_CONFIG_MESA;

	renderer->egl_context = eglCreateContext(renderer->egl_display, egl_config, EGL_NO_CONTEXT, attribs);
	if (renderer->egl_context == NULL) {
		nemolog_error("GLRENDERER", "failed to create egl context\n");
		return -1;
	}

	r = eglMakeCurrent(renderer->egl_display, egl_surface, egl_surface, renderer->egl_context);
	if (r == EGL_FALSE) {
		nemolog_error("GLRENDERER", "failed to make egl context current\n");
		return -1;
	}

	vendor = (const char *)glGetString(GL_VENDOR);
	if (vendor != NULL)
		nemolog_message("GLRENDERER", "OpenGL ES vendor: %s\n", vendor);

	version = (const char *)glGetString(GL_VERSION);
	if (version != NULL)
		nemolog_message("GLRENDERER", "OpenGL ES version: %s\n", version);

	extensions = (const char *)glGetString(GL_EXTENSIONS);
	if (extensions == NULL) {
		nemolog_error("GLRENDERER", "failed to retrieve gl extension string\n");
		return -1;
	}

	if (!strstr(extensions, "GL_EXT_texture_format_BGRA8888")) {
		nemolog_error("GLRENDERER", "GL_EXT_texture_format_BGRA8888 not available\n");
		return -1;
	}

	if (strstr(extensions, "GL_EXT_read_format_bgra"))
		screen->read_format = PIXMAN_a8r8g8b8;
	else
		screen->read_format = PIXMAN_a8b8g8r8;

	if (strstr(extensions, "GL_EXT_unpack_subimage"))
		nemolog_message("GLRENDERER", "OpenGL has unpack subimage extension\n");
	else
		nemolog_message("GLRENDERER", "OpenGL has no unpack subimage extension\n");

	if (strstr(extensions, "GL_OES_EGL_image_external"))
		renderer->has_egl_image_external = 1;

	renderer->texture_shader_rgba.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->texture_shader_rgba.fragment_source = GLCOMPZ_TEXTURE_FRAGMENT_SHADER_RGBA;
	renderer->texture_shader_rgbx.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->texture_shader_rgbx.fragment_source = GLCOMPZ_TEXTURE_FRAGMENT_SHADER_RGBX;
	renderer->texture_shader_egl_external.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->texture_shader_egl_external.fragment_source = GLCOMPZ_TEXTURE_FRAGMENT_SHADER_EGL_EXTERNAL;
	renderer->texture_shader_y_uv.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->texture_shader_y_uv.fragment_source = GLCOMPZ_TEXTURE_FRAGMENT_SHADER_Y_UV;
	renderer->texture_shader_y_u_v.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->texture_shader_y_u_v.fragment_source = GLCOMPZ_TEXTURE_FRAGMENT_SHADER_Y_U_V;
	renderer->texture_shader_y_xuxv.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->texture_shader_y_xuxv.fragment_source = GLCOMPZ_TEXTURE_FRAGMENT_SHADER_Y_XUXV;
	renderer->solid_shader.vertex_source = GLCOMPZ_VERTEX_SHADER;
	renderer->solid_shader.fragment_source = GLCOMPZ_SOLID_FRAGMENT_SHADER;

	return 0;
}

int glrenderer_prepare_screen(struct nemorenderer *base, struct nemoscreen *screen, EGLNativeWindowType window, const EGLint *visualid)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glsurface *surface;
	EGLConfig egl_config;
	int i;

	if (glrenderer_choose_config(renderer, renderer->attribs, visualid, &egl_config) < 0) {
		nemolog_error("GLRENDERER", "failed to choose egl config\n");
		return -1;
	}

	if (egl_config != renderer->egl_config && renderer->has_configless_context == 0) {
		nemolog_error("GLRENDERER", "different egl config is not supported\n");
		return -1;
	}

	surface = (struct glsurface *)malloc(sizeof(struct glsurface));
	if (surface == NULL)
		return -1;
	memset(surface, 0, sizeof(struct glsurface));

	surface->egl_surface = eglCreateWindowSurface(renderer->egl_display, egl_config, window, NULL);
	if (surface->egl_surface == EGL_NO_SURFACE) {
		nemolog_error("GLRENDERER", "failed to create egl surface\n");
		goto err1;
	}

	if (renderer->egl_context == NULL) {
		if (glrenderer_prepare_egl_context(renderer, screen, surface->egl_surface) < 0)
			goto err1;
	}

	for (i = 0; i < GLRENDERER_BUFFER_AGE_COUNT; i++)
		pixman_region32_init(&surface->damages[i]);

	screen->gcontext = (void *)surface;

	return 0;

err1:
	free(surface);

	return -1;
}

void glrenderer_finish_screen(struct nemorenderer *base, struct nemoscreen *screen)
{
	struct glrenderer *renderer = (struct glrenderer *)container_of(base, struct glrenderer, base);
	struct glsurface *surface = (struct glsurface *)screen->gcontext;
	int i;

	for (i = 0; i < GLRENDERER_BUFFER_AGE_COUNT; i++)
		pixman_region32_fini(&surface->damages[i]);

	eglDestroySurface(renderer->egl_display, surface->egl_surface);

	free(surface);
}
