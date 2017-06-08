#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <gbm.h>
#include <drm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <wayland-server.h>
#include <wayland-presentation-timing-server-protocol.h>

#include <drmnode.h>
#include <glrenderer.h>
#include <pixmanrenderer.h>
#include <renderer.h>
#include <compz.h>
#include <screen.h>
#include <renderer.h>
#include <view.h>
#include <canvas.h>
#include <nemomisc.h>
#include <nemolog.h>

static void drm_destroy_frame_callback(struct gbm_bo *bo, void *data)
{
	struct drmframe *frame = (struct drmframe *)data;
	struct gbm_device *gbm = gbm_bo_get_device(bo);

	if (frame->id != 0)
		drmModeRmFB(gbm_device_get_fd(gbm), frame->id);

	nemobuffer_reference(&frame->buffer_reference, NULL);

	free(frame);
}

static struct drmframe *drm_create_dumb_frame(struct drmnode *node, int width, int height)
{
	struct drm_mode_create_dumb create_arg;
	struct drm_mode_destroy_dumb destroy_arg;
	struct drm_mode_map_dumb map_arg;
	struct drmframe *frame;
	int r;

	frame = (struct drmframe *)malloc(sizeof(struct drmframe));
	if (frame == NULL)
		return NULL;
	memset(frame, 0, sizeof(struct drmframe));

	memset(&create_arg, 0, sizeof(create_arg));
	create_arg.bpp = 32;
	create_arg.width = width;
	create_arg.height = height;

	r = drmIoctl(node->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg);
	if (r < 0)
		goto err1;

	frame->handle = create_arg.handle;
	frame->stride = create_arg.pitch;
	frame->size = create_arg.size;
	frame->fd = node->fd;

	r = drmModeAddFB(node->fd, width, height, 24, 32, frame->stride, frame->handle, &frame->id);
	if (r < 0)
		goto err2;

	memset(&map_arg, 0, sizeof(map_arg));
	map_arg.handle = frame->handle;
	r = drmIoctl(node->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_arg);
	if (r < 0)
		goto err3;

	frame->map = mmap(0, frame->size, PROT_WRITE, MAP_SHARED, node->fd, map_arg.offset);
	if (frame->map == MAP_FAILED)
		goto err3;

	return frame;

err3:
	drmModeRmFB(node->fd, frame->id);

err2:
	memset(&destroy_arg, 0, sizeof(destroy_arg));
	destroy_arg.handle = create_arg.handle;
	drmIoctl(node->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg);

err1:
	free(frame);

	return NULL;
}

static void drm_destroy_dumb_frame(struct drmframe *frame)
{
	struct drm_mode_destroy_dumb destroy_arg;

	if (frame->map == NULL)
		return;

	if (frame->id != 0)
		drmModeRmFB(frame->fd, frame->id);

	nemobuffer_reference(&frame->buffer_reference, NULL);

	munmap(frame->map, frame->size);

	memset(&destroy_arg, 0, sizeof(destroy_arg));
	destroy_arg.handle = frame->handle;
	drmIoctl(frame->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg);

	free(frame);
}

static struct drmframe *drm_get_frame_from_bo(struct gbm_bo *bo, struct drmnode *node, uint32_t format)
{
	struct drmframe *frame = gbm_bo_get_user_data(bo);
	uint32_t width, height;
	uint32_t handles[4] = { 0 }, pitches[4] = { 0 }, offsets[4] = { 0 };
	int r = -1;

	if (frame != NULL)
		return frame;

	frame = (struct drmframe *)malloc(sizeof(struct drmframe));
	if (frame == NULL)
		return NULL;
	memset(frame, 0, sizeof(struct drmframe));

	frame->bo = bo;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);

	frame->stride = gbm_bo_get_stride(bo);
	frame->handle = gbm_bo_get_handle(bo).u32;
	frame->size = frame->stride * height;
	frame->fd = node->fd;

	if (node->min_width > width || width > node->max_width || node->min_height > height || height > node->max_height) {
		nemolog_error("DRM", "bo geometry out of bounds\n");
		goto err1;
	}

	if (format && !node->no_addfb2) {
		handles[0] = frame->handle;
		pitches[0] = frame->stride;
		offsets[0] = 0;

		r = drmModeAddFB2(node->fd, width, height, format, handles, pitches, offsets, &frame->id, 0);
		if (r < 0) {
			node->no_addfb2 = 1;
		}
	}

	if (r < 0)
		r = drmModeAddFB(node->fd, width, height, 24, 32, frame->stride, frame->handle, &frame->id);

	if (r < 0) {
		nemolog_error("DRM", "failed to create framebuffer\n");
		goto err1;
	}

	gbm_bo_set_user_data(bo, frame, drm_destroy_frame_callback);

	return frame;

err1:
	free(frame);

	return NULL;
}

static void drm_release_screen_frame(struct drmscreen *screen, struct drmframe *frame)
{
	if (frame == NULL)
		return;

	if (frame->map && (frame != screen->dumbs[0] && frame != screen->dumbs[1])) {
		drm_destroy_dumb_frame(frame);
	} else if (frame->bo != NULL) {
		if (frame->is_client_buffer != 0)
			gbm_bo_destroy(frame->bo);
		else
			gbm_surface_release_buffer(screen->surface, frame->bo);
	}
}

static void drm_handle_page_flip(int fd, unsigned int frame, unsigned int secs, unsigned int usecs, void *data)
{
	struct drmscreen *screen = (struct drmscreen *)data;

	screen->base.msc += frame;

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	wl_event_source_timer_update(screen->pageflip_timer, 0);
#endif

	if (screen->pageflip_pending != 0) {
		drm_release_screen_frame(screen, screen->current);

		screen->current = screen->next;
		screen->next = NULL;

		screen->pageflip_pending = 0;
	}

	nemoscreen_finish_frame(&screen->base, secs, usecs,
			PRESENTATION_FEEDBACK_KIND_VSYNC |
			PRESENTATION_FEEDBACK_KIND_HW_COMPLETION |
			PRESENTATION_FEEDBACK_KIND_HW_CLOCK);
}

static void drm_handle_vblank(int fd, unsigned int frame, unsigned int secs, unsigned int usecs, void *data)
{
	struct drmscreen *screen = (struct drmscreen *)data;
}

static int drm_dispatch_event(int fd, uint32_t mask, void *data)
{
	drmEventContext ctx;

	memset(&ctx, 0, sizeof(ctx));
	ctx.version = DRM_EVENT_CONTEXT_VERSION;
	ctx.page_flip_handler = drm_handle_page_flip;
	ctx.vblank_handler = drm_handle_vblank;
	drmHandleEvent(fd, &ctx);

	return 1;
}

static int drm_prepare_pixman(struct drmnode *node)
{
	node->base.pixman = pixmanrenderer_create(&node->base);
	if (node->base.pixman == NULL)
		return -1;

	return 0;
}

static void drm_finish_pixman(struct drmnode *node)
{
	if (node->base.pixman == NULL)
		return;

	pixmanrenderer_destroy(node->base.pixman);
}

static int drm_prepare_pixman_screen(struct drmscreen *screen)
{
	struct drmnode *node = screen->node;
	int width, height;
	int i;

	width = screen->base.current_mode->width;
	height = screen->base.current_mode->height;

	for (i = 0; i < ARRAY_LENGTH(screen->dumbs); i++) {
		screen->dumbs[i] = drm_create_dumb_frame(screen->node, width, height);
		if (screen->dumbs[i] == NULL)
			goto err1;

		screen->images[i] = pixman_image_create_bits(PIXMAN_x8r8g8b8,
				width, height,
				screen->dumbs[i]->map,
				screen->dumbs[i]->stride);
	}

	if (pixmanrenderer_prepare_screen(node->base.pixman, &screen->base) < 0) {
		nemolog_error("DRM", "failed to prepare pixman renderer screen\n");
		goto err1;
	}

	pixman_region32_init_rect(&screen->damage,
			screen->base.x, screen->base.y,
			screen->base.width, screen->base.height);

	return 0;

err1:
	for (i = 0; i < ARRAY_LENGTH(screen->dumbs); i++) {
		if (screen->dumbs[i])
			drm_destroy_dumb_frame(screen->dumbs[i]);
		if (screen->images[i])
			pixman_image_unref(screen->images[i]);

		screen->dumbs[i] = NULL;
		screen->images[i] = NULL;
	}

	return -1;
}

static void drm_finish_pixman_screen(struct drmscreen *screen)
{
	struct drmnode *node = screen->node;
	int i;

	pixmanrenderer_finish_screen(node->base.pixman, &screen->base);

	pixman_region32_fini(&screen->damage);

	for (i = 0; i < ARRAY_LENGTH(screen->dumbs); i++) {
		if (screen->dumbs[i])
			drm_destroy_dumb_frame(screen->dumbs[i]);
		if (screen->images[i])
			pixman_image_unref(screen->images[i]);
	}
}

static int drm_prepare_egl(struct drmnode *node)
{
	node->gbm = gbm_create_device(node->fd);
	if (node->gbm == NULL)
		return -1;

	node->base.opengl = glrenderer_create(&node->base, node->gbm, &node->format);
	if (node->base.opengl == NULL)
		goto err1;

	return 0;

err1:
	gbm_device_destroy(node->gbm);
	node->gbm = NULL;

	return -1;
}

static void drm_finish_egl(struct drmnode *node)
{
	if (node->base.opengl == NULL)
		return;

	glrenderer_destroy(node->base.opengl);
}

static int drm_prepare_egl_screen(struct drmscreen *screen)
{
	struct drmnode *node = screen->node;

	screen->surface = gbm_surface_create(node->gbm,
			screen->base.current_mode->width,
			screen->base.current_mode->height,
			screen->format,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (screen->surface == NULL) {
		nemolog_error("DRM", "failed to create gbm surface\n");
		return -1;
	}

	if (glrenderer_prepare_screen(node->base.opengl, &screen->base, screen->surface, &screen->format) < 0) {
		nemolog_error("DRM", "failed to prepare gl renderer screen\n");
		goto err1;
	}

	return 0;

err1:
	gbm_surface_destroy(screen->surface);
	screen->surface = NULL;

	return -1;
}

static void drm_finish_egl_screen(struct drmscreen *screen)
{
	struct drmnode *node = screen->node;

	glrenderer_finish_screen(node->base.opengl, &screen->base);

	gbm_surface_destroy(screen->surface);
}

static int drm_find_crtc_for_connector(struct drmnode *node, drmModeRes *resources, drmModeConnector *connector)
{
	drmModeEncoder *encoder;
	uint32_t crtcs;
	int i, j;

	for (j = 0; j < connector->count_encoders; j++) {
		encoder = drmModeGetEncoder(node->fd, connector->encoders[j]);
		if (encoder == NULL) {
			nemolog_error("DRM", "failed to get encoder\n");
			return -1;
		}
		crtcs = encoder->possible_crtcs;
		drmModeFreeEncoder(encoder);

		for (i = 0; i < resources->count_crtcs; i++) {
			if (crtcs & (1 << i) &&
					!(node->crtc_allocator & (1 << resources->crtcs[i])))
				return i;
		}
	}

	return -1;
}

static int drm_subpixel_to_wayland(int value)
{
	switch (value) {
		case DRM_MODE_SUBPIXEL_NONE:
			return WL_OUTPUT_SUBPIXEL_NONE;
		case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
			return WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;
		case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
			return WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR;
		case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
			return WL_OUTPUT_SUBPIXEL_VERTICAL_RGB;
		case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
			return WL_OUTPUT_SUBPIXEL_VERTICAL_BGR;
		default:
			return WL_OUTPUT_SUBPIXEL_UNKNOWN;
	}

	return WL_OUTPUT_SUBPIXEL_UNKNOWN;
}

static drmModePropertyPtr drm_get_property(int fd, drmModeConnectorPtr connector, const char *name)
{
	drmModePropertyPtr prop;
	int i;

	for (i = 0; i < connector->count_props; i++) {
		prop = drmModeGetProperty(fd, connector->props[i]);
		if (prop == NULL)
			continue;

		if (!strcmp(prop->name, name))
			return prop;

		drmModeFreeProperty(prop);
	}

	return NULL;
}

static struct drmmode *drm_create_mode(drmModeModeInfo *info)
{
	struct drmmode *mode;
	uint64_t refresh;

	mode = (struct drmmode *)malloc(sizeof(struct drmmode));
	if (mode == NULL)
		return NULL;
	memset(mode, 0, sizeof(struct drmmode));

	wl_list_init(&mode->base.link);

	mode->base.flags = 0;
	mode->base.width = info->hdisplay;
	mode->base.height = info->vdisplay;

	refresh = (info->clock * 1000000LL / info->htotal + info->vtotal / 2) / info->vtotal;

	if (info->flags & DRM_MODE_FLAG_INTERLACE)
		refresh *= 2;
	if (info->flags & DRM_MODE_FLAG_DBLSCAN)
		refresh /= 2;
	if (info->vscan > 1)
		refresh /= info->vscan;

	mode->base.refresh = refresh;
	mode->modeinfo = *info;

	if (info->type & DRM_MODE_TYPE_PREFERRED)
		mode->base.flags |= WL_OUTPUT_MODE_PREFERRED;

	return mode;
}

static void drm_destroy_mode(struct drmmode *mode)
{
	wl_list_remove(&mode->base.link);

	free(mode);
}

static struct drmmode *drm_choose_mode(struct drmscreen *screen, struct nemomode *mode)
{
	struct drmmode *tmode = NULL;
	struct drmmode *dmode;

	if ((screen->base.current_mode->width == mode->width) &&
			(screen->base.current_mode->height == mode->height) &&
			(screen->base.current_mode->refresh == mode->refresh || mode->refresh == 0)) {
		return (struct drmmode *)screen->base.current_mode;
	}

	wl_list_for_each(dmode, &screen->base.mode_list, base.link) {
		if (dmode->modeinfo.hdisplay == mode->width &&
				dmode->modeinfo.vdisplay == mode->height) {
			if (dmode->modeinfo.vrefresh == mode->refresh || mode->refresh == 0) {
				return dmode;
			} else if (tmode == NULL) {
				tmode = dmode;
			}
		}
	}

	return tmode;
}

static void drm_screen_repaint(struct nemoscreen *base)
{
	struct drmscreen *screen = (struct drmscreen *)container_of(base, struct drmscreen, base);
	struct drmnode *node = screen->node;
	struct timespec ts;

	if (screen->pageflip_pending != 0)
		goto postpone_frame;

	if (screen->current == NULL)
		goto finish_frame;

	if (drmModePageFlip(node->fd, screen->crtc_id,
				screen->current->id,
				DRM_MODE_PAGE_FLIP_EVENT, screen) < 0) {
		nemolog_error("DRM", "failed to queue pageflip current: devnode(%s), errno(%d)\n", node->devnode, errno);
		goto finish_frame;
	}

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	wl_event_source_timer_update(screen->pageflip_timer, node->pageflip_timeout);
#endif

postpone_frame:
	return;

finish_frame:
	clock_gettime(screen->node->clock, &ts);
	nemoscreen_finish_frame(base, ts.tv_sec, ts.tv_nsec / 1000, 0x0);
}

static void drm_screen_render_pixman(struct drmscreen *screen, pixman_region32_t *damage)
{
	struct drmnode *node = screen->node;
	pixman_region32_t total_damage;

	pixman_region32_init(&total_damage);

	pixman_region32_union(&total_damage, damage, &screen->damage);
	pixman_region32_copy(&screen->damage, damage);

	screen->current_image ^= 1;

	screen->next = screen->dumbs[screen->current_image];

	pixmanrenderer_set_screen_buffer(node->base.pixman, &screen->base, screen->images[screen->current_image]);

	node->base.pixman->repaint_screen(node->base.pixman, &screen->base, &total_damage);

	pixman_region32_fini(&total_damage);
}

static void drm_screen_render_gl(struct drmscreen *screen, pixman_region32_t *damage)
{
	struct drmnode *node = screen->node;
	struct gbm_bo *bo;

	node->base.opengl->repaint_screen(node->base.opengl, &screen->base, damage);

	bo = gbm_surface_lock_front_buffer(screen->surface);
	if (bo == NULL) {
		nemolog_error("DRM", "failed to lock front buffer\n");
		return;
	}

	screen->next = drm_get_frame_from_bo(bo, node, screen->format);
	if (screen->next == NULL) {
		nemolog_error("DRM", "failed to get frame for bo\n");
		gbm_surface_release_buffer(screen->surface, bo);
		return;
	}
}

static void drm_screen_render_overlay(struct drmscreen *screen, pixman_region32_t *damage)
{
	struct drmnode *node = screen->node;
	struct nemobuffer *buffer = screen->base.overlay->canvas->buffer_reference.buffer;
	struct gbm_bo *bo;

	bo = gbm_bo_import(node->gbm, GBM_BO_IMPORT_WL_BUFFER, buffer->resource, GBM_BO_USE_SCANOUT);
	if (bo == NULL) {
		nemolog_error("DRM", "failed to lock front buffer\n");
		return;
	}

	screen->next = drm_get_frame_from_bo(bo, node, screen->format);
	if (screen->next == NULL) {
		nemolog_error("DRM", "failed to get frame for bo\n");
		gbm_bo_destroy(bo);
		return;
	}

	assert(screen->next->buffer_reference.buffer == NULL);

	screen->next->is_client_buffer = 1;

	nemobuffer_reference(&screen->next->buffer_reference, buffer);
}

static void drm_screen_render_frame(struct drmscreen *screen, pixman_region32_t *damage)
{
	struct nemocompz *compz = screen->base.compz;

	if (nemoscreen_has_state(&screen->base, NEMOSCREEN_OVERLAY_STATE) == 0) {
		if (compz->use_pixman != 0) {
			drm_screen_render_pixman(screen, damage);
		} else {
			drm_screen_render_gl(screen, damage);
		}
	} else {
		drm_screen_render_overlay(screen, damage);
	}

	pixman_region32_subtract(&screen->base.damage, &screen->base.damage, damage);
}

static int drm_screen_repaint_frame(struct nemoscreen *base, pixman_region32_t *damage)
{
	struct drmscreen *screen = (struct drmscreen *)container_of(base, struct drmscreen, base);
	struct drmnode *node = screen->node;

	if (screen->next == NULL)
		drm_screen_render_frame(screen, damage);
	if (screen->next == NULL)
		return -1;

	if (screen->current == NULL ||
			screen->current->stride != screen->next->stride) {
		struct drmmode *mode = (struct drmmode *)container_of(screen->base.current_mode, struct drmmode, base);

		if (drmModeSetCrtc(node->fd, screen->crtc_id,
					screen->next->id, 0, 0,
					&screen->connector_id, 1,
					&mode->modeinfo) < 0) {
			nemolog_error("DRM", "failed to set mode: devnode(%s), errno(%d)\n", node->devnode, errno);
			goto err1;
		}

		base->set_dpms(base, NEMODPMS_ON_STATE);
	}

	if (drmModePageFlip(node->fd, screen->crtc_id,
				screen->next->id,
				DRM_MODE_PAGE_FLIP_EVENT, screen) < 0) {
		nemolog_error("DRM", "failed to queue pageflip next: devnode(%s), errno(%d)\n", node->devnode, errno);
		goto err1;
	}

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	wl_event_source_timer_update(screen->pageflip_timer, node->pageflip_timeout);
#endif

	screen->pageflip_pending = 1;

	return 0;

err1:
	if (screen->next != NULL) {
		drm_release_screen_frame(screen, screen->next);
		screen->next = NULL;
	}

	return -1;
}

static int drm_screen_read_pixels(struct nemoscreen *base, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct nemocompz *compz = base->compz;

	if (compz->use_pixman != 0) {
		base->node->pixman->read_pixels(base->node->pixman, base, format, pixels, x, y, width, height);
	} else {
		base->node->opengl->read_pixels(base->node->opengl, base, format, pixels, x, y, width, height);
	}

	return 0;
}

static void drm_screen_destroy(struct nemoscreen *base)
{
	struct drmscreen *screen = (struct drmscreen *)container_of(base, struct drmscreen, base);
	struct drmnode *node = screen->node;
	struct nemomode *mode, *next;
	drmModeCrtcPtr crtc = screen->orig_crtc;

	nemolog_message("DRM", "destroy %d:%d drm screen\n", node->base.nodeid, screen->base.screenid);

	wl_list_for_each_safe(mode, next, &screen->base.mode_list, link) {
		drm_destroy_mode((struct drmmode *)container_of(mode, struct drmmode, base));
	}

	drmModeFreeProperty(screen->dpms_prop);

	drmModeSetCursor(node->fd, screen->crtc_id, 0, 0, 0);

	drmModeSetCrtc(node->fd, crtc->crtc_id, crtc->buffer_id, crtc->x, crtc->y, &screen->connector_id, 1, &crtc->mode);
	drmModeFreeCrtc(crtc);

	if (base->compz->use_pixman != 0) {
		drm_finish_pixman_screen(screen);
	} else {
		drm_finish_egl_screen(screen);
	}

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	wl_event_source_remove(screen->pageflip_timer);
#endif

	wl_list_remove(&base->link);

	nemoscreen_finish(base);

	free(screen);
}

static void drm_screen_set_dpms(struct nemoscreen *base, int level)
{
	struct drmscreen *screen = (struct drmscreen *)container_of(base, struct drmscreen, base);

	if (screen->dpms_prop == NULL)
		return;

	drmModeConnectorSetProperty(screen->node->fd,
			screen->connector_id,
			screen->dpms_prop->prop_id,
			level);
}

static int drm_screen_switch_mode(struct nemoscreen *base, struct nemomode *mode)
{
	struct nemocompz *compz;
	struct drmscreen *screen;
	struct drmmode *dmode;

	if (base == NULL || mode == NULL)
		return -1;

	screen = (struct drmscreen *)container_of(base, struct drmscreen, base);
	compz = screen->node->base.compz;
	dmode = drm_choose_mode(screen, mode);
	if (dmode == NULL) {
		nemolog_error("DRM", "invalid resolution: %dx%d\n", mode->width, mode->height);
		return -1;
	}

	if (&dmode->base == screen->base.current_mode)
		return 0;

	screen->base.current_mode->flags = 0;
	screen->base.current_mode = &dmode->base;
	screen->base.current_mode->flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;

	drm_release_screen_frame(screen, screen->current);
	drm_release_screen_frame(screen, screen->next);

	screen->current = NULL;
	screen->next = NULL;

	if (compz->use_pixman != 0) {
		drm_finish_pixman_screen(screen);

		if (drm_prepare_pixman_screen(screen) < 0) {
			nemolog_error("DRM", "failed to prepare pixman screen\n");
			goto err1;
		}
	} else {
		drm_finish_egl_screen(screen);

		if (drm_prepare_egl_screen(screen) < 0) {
			nemolog_error("DRM", "failed to prepare egl screen\n");
			goto err1;
		}
	}

	return 0;

err1:
	screen->base.current_mode->flags = 0;
	screen->base.current_mode = NULL;

	return -1;
}

static void drm_screen_set_gamma(struct nemoscreen *base, uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b)
{
}

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
static int drm_screen_dispatch_pageflip_timeout(void *data)
{
	struct drmscreen *screen = (struct drmscreen *)data;
	struct drmnode *node = screen->node;
	struct nemocompz *compz = node->base.compz;

	nemolog_error("DRM", "pageflip timeout on devnode(%s)\n", node->devnode);

	nemocompz_exit(compz);

	return 1;
}
#endif

static int drm_create_screen_for_connector(struct drmnode *node, drmModeRes *resources, drmModeConnector *connector)
{
	static const char *connector_names[] = {
		"None",
		"VGA",
		"DVI",
		"DVI",
		"DVI",
		"Composite",
		"TV",
		"LVDS",
		"CTV",
		"DIN",
		"DP",
		"HDMI",
		"HDMI",
		"TV",
		"eDP",
	};
	struct nemocompz *compz = node->base.compz;
	struct drmscreen *screen;
	struct drmmode *mode;
	drmModeEncoder *encoder;
	drmModeModeInfo crtcmode;
	drmModeCrtc *crtc;
	const char *typename;
	int index = -1;
	int i;

	index = drm_find_crtc_for_connector(node, resources, connector);
	if (index < 0) {
		nemolog_error("DRM", "No found usable crtc/encoder pair for connector\n");
		return -1;
	}

	screen = (struct drmscreen *)malloc(sizeof(struct drmscreen));
	if (screen == NULL)
		return -1;
	memset(screen, 0, sizeof(struct drmscreen));

	screen->base.compz = compz;
	screen->base.node = &node->base;
	screen->base.screenid = index;
	screen->base.subpixel = drm_subpixel_to_wayland(connector->subpixel);
	screen->base.make = "unknown";
	screen->base.model = "unknown";
	screen->base.serial = "unknown";

	wl_list_init(&screen->base.mode_list);

#ifdef NEMOUX_DRM_PAGEFLIP_TIMEOUT
	screen->pageflip_timer = wl_event_loop_add_timer(compz->loop, drm_screen_dispatch_pageflip_timeout, screen);
#endif

	if (connector->connector_type < ARRAY_LENGTH(connector_names))
		typename = connector_names[connector->connector_type];
	else
		typename = "unknown";
	snprintf(screen->base.name, sizeof(screen->base.name), "%s%d", typename, connector->connector_type_id);

	screen->node = node;
	screen->pipe = index;
	screen->format = node->format;
	screen->crtc_id = resources->crtcs[index];
	screen->connector_id = connector->connector_id;

	node->crtc_allocator |= (1 << screen->crtc_id);
	node->connector_allocator |= (1 << screen->connector_id);

	screen->orig_crtc = drmModeGetCrtc(node->fd, screen->crtc_id);
	screen->dpms_prop = drm_get_property(node->fd, connector, "DPMS");

	memset(&crtcmode, 0, sizeof(crtcmode));

	encoder = drmModeGetEncoder(node->fd, connector->encoder_id);
	if (encoder != NULL) {
		crtc = drmModeGetCrtc(node->fd, encoder->crtc_id);
		drmModeFreeEncoder(encoder);
		if (crtc == NULL)
			goto err1;
		if (crtc->mode_valid)
			crtcmode = crtc->mode;
		drmModeFreeCrtc(crtc);
	}

	for (i = 0; i < connector->count_modes; i++) {
		mode = drm_create_mode(&connector->modes[i]);
		if (mode == NULL)
			goto err1;

		nemolog_message("DRM", "[%02d mode] %d x %d (%d)\n", i, mode->base.width, mode->base.height, mode->base.refresh);

		if (!memcmp(&crtcmode, &mode->modeinfo, sizeof(crtcmode))) {
			screen->base.current_mode = &mode->base;
		}

		if (screen->base.current_mode == NULL || mode->base.flags & WL_OUTPUT_MODE_PREFERRED) {
			screen->base.current_mode = &mode->base;
		}

		wl_list_insert(screen->base.mode_list.prev, &mode->base.link);
	}

	if (screen->base.current_mode == NULL) {
		nemolog_error("DRM", "no available mode for %s\n", screen->base.name);
		goto err1;
	}

	screen->base.width = screen->base.current_mode->width;
	screen->base.height = screen->base.current_mode->height;
	screen->base.current_mode->flags |= WL_OUTPUT_MODE_CURRENT;

	nemolog_message("DRM", "screen(%d, %d) x = %d, y = %d, width = %d, height = %d\n",
			node->base.nodeid,
			screen->base.screenid,
			screen->base.x,
			screen->base.y,
			screen->base.width,
			screen->base.height);

	screen->base.mmwidth = connector->mmWidth;
	screen->base.mmheight = connector->mmHeight;

	nemoscreen_prepare(&screen->base);

	if (compz->use_pixman != 0) {
		if (drm_prepare_pixman_screen(screen) < 0) {
			nemolog_error("DRM", "failed to prepare pixman screen\n");
			goto err1;
		}
	} else {
		if (drm_prepare_egl_screen(screen) < 0) {
			nemolog_error("DRM", "failed to prepare egl screen\n");
			goto err1;
		}
	}

	screen->base.repaint = drm_screen_repaint;
	screen->base.repaint_frame = drm_screen_repaint_frame;
	screen->base.read_pixels = drm_screen_read_pixels;
	screen->base.destroy = drm_screen_destroy;
	screen->base.set_dpms = drm_screen_set_dpms;
	screen->base.switch_mode = drm_screen_switch_mode;
	screen->base.set_gamma = drm_screen_set_gamma;

	screen->base.gamma_size = screen->orig_crtc->gamma_size;

	wl_list_insert(compz->screen_list.prev, &screen->base.link);

	nemoscreen_update_transform(&screen->base);
	nemocompz_update_scene(compz);

	nemolog_message("DRM", "screen %s, (connector %d, crtc %d)\n",
			screen->base.name, screen->connector_id, screen->crtc_id);

	return 0;

err1:
	drmModeFreeCrtc(screen->orig_crtc);
	node->crtc_allocator &= ~(1 << screen->crtc_id);
	node->connector_allocator &= ~(1 << screen->connector_id);
	free(screen);

	return -1;
}

static int drm_prepare_screens(struct drmnode *node)
{
	drmModeConnector *connector;
	drmModeRes *resources;
	int i;

	resources = drmModeGetResources(node->fd);
	if (resources == NULL) {
		nemolog_error("DRM", "failed to get mode resources\n");
		return -1;
	}

	node->crtcs = (uint32_t *)malloc(sizeof(uint32_t) * resources->count_crtcs);
	if (node->crtcs == NULL)
		goto err1;
	memcpy(node->crtcs, resources->crtcs, sizeof(uint32_t) * resources->count_crtcs);
	node->ncrtcs = resources->count_crtcs;

	node->min_width = resources->min_width;
	node->max_width = resources->max_width;
	node->min_height = resources->min_height;
	node->max_height = resources->max_height;

	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(node->fd, resources->connectors[i]);
		if (connector == NULL)
			continue;

		if (connector->connection == DRM_MODE_CONNECTED) {
			drm_create_screen_for_connector(node, resources, connector);
		}

		drmModeFreeConnector(connector);
	}

	drmModeFreeResources(resources);

	return 0;

err1:
	drmModeFreeResources(resources);

	return -1;
}

static void drm_handle_session(struct wl_listener *listener, void *data)
{
	struct drmnode *node = (struct drmnode *)container_of(listener, struct drmnode, session_listener);
	struct nemocompz *compz = (struct nemocompz *)data;

	if (compz->session_active == 0) {
		if (node->has_master != 0) {
			drmDropMaster(node->fd);
			node->has_master = 0;
		}
	} else {
		if (node->has_master == 0) {
			drmSetMaster(node->fd);
			node->has_master = 1;
		}
	}
}

struct drmnode *drm_create_node(struct nemocompz *compz, uint32_t nodeid, const char *path, int fd)
{
	struct drmnode *node;
	uint64_t cap;
	int r;

	nemolog_message("DRM", "create %s drm node\n", path);

	node = (struct drmnode *)malloc(sizeof(struct drmnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct drmnode));

	node->base.compz = compz;
	node->base.nodeid = nodeid;
	node->base.pixman = NULL;
	node->base.opengl = NULL;
	node->fd = fd;
	node->has_master = 0;
	node->devnode = strdup(path);

	wl_list_init(&node->base.link);
	wl_list_init(&node->session_listener.link);

	rendernode_prepare(&node->base);
	if (node->base.id >= compz->nodemax)
		goto err1;

	r = drmGetCap(node->fd, DRM_CAP_TIMESTAMP_MONOTONIC, &cap);
	if (r == 0 && cap == 1)
		node->clock = CLOCK_MONOTONIC;
	else
		node->clock = CLOCK_REALTIME;

	r = nemocompz_set_presentation_clock(compz, node->clock);
	if (r < 0)
		goto err1;

	node->format = GBM_FORMAT_XRGB8888;

	node->source = wl_event_loop_add_fd(compz->loop,
			node->fd,
			WL_EVENT_READABLE,
			drm_dispatch_event,
			node);
	if (node->source == NULL)
		goto err1;

	if (compz->use_pixman != 0) {
		if (drm_prepare_pixman(node) < 0) {
			nemolog_error("DRM", "failed to prepare pixman renderer\n");
			goto err1;
		}
	} else {
		if (drm_prepare_egl(node) < 0) {
			nemolog_error("DRM", "failed to prepare egl renderer\n");
			goto err1;
		}
	}

	drm_prepare_screens(node);

	node->session_listener.notify = drm_handle_session;
	wl_signal_add(&compz->session_signal, &node->session_listener);

	wl_list_insert(compz->render_list.prev, &node->base.link);

	return node;

err1:
	drm_destroy_node(node);

	return NULL;
}

void drm_destroy_node(struct drmnode *node)
{
	nemolog_message("DRM", "destroy %s drm node\n", node->devnode);

	wl_list_remove(&node->base.link);

	wl_list_remove(&node->session_listener.link);

	if (node->source != NULL)
		wl_event_source_remove(node->source);

	drm_finish_pixman(node);
	drm_finish_egl(node);

	close(node->fd);

	rendernode_finish(&node->base);

	free(node->devnode);
	free(node);
}
