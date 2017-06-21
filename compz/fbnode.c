#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <time.h>
#include <pixman.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include <wayland-server.h>
#include <wayland-presentation-timing-server-protocol.h>

#include <fbnode.h>
#include <compz.h>
#include <screen.h>
#include <pixmanrenderer.h>
#include <nemomisc.h>
#include <nemolog.h>

static void fb_screen_repaint(struct nemoscreen *base)
{
	uint32_t msecs;

	msecs = time_current_msecs();
	nemoscreen_finish_frame(base, msecs / 1000, (msecs % 1000) * 1000, 0x0);
}

static int fb_screen_repaint_frame(struct nemoscreen *base, pixman_region32_t *damage)
{
	struct fbscreen *screen = (struct fbscreen *)container_of(base, struct fbscreen, base);
	struct fbnode *node = screen->node;
	pixman_box32_t *boxes;
	int nboxes, width, height, i;
	int x1, x2, y1, y2;

	pixmanrenderer_set_screen_buffer(node->base.pixman, &screen->base, screen->shadow_image);

	node->base.pixman->repaint_screen(node->base.pixman, &screen->base, damage);

	width = pixman_image_get_width(screen->shadow_image);
	height = pixman_image_get_height(screen->shadow_image);
	boxes = pixman_region32_rectangles(damage, &nboxes);

	for (i = 0; i < nboxes; i++) {
		x1 = boxes[i].x1;
		x2 = boxes[i].x2;
		y1 = boxes[i].y1;
		y2 = boxes[i].y2;

		pixman_image_composite32(PIXMAN_OP_SRC,
				screen->shadow_image,
				NULL,
				screen->screen_image,
				x1, y1,
				0, 0,
				x1, y1,
				x2 - x1,
				y2 - y1);
	}

	pixman_region32_subtract(&screen->base.damage, &screen->base.damage, damage);

	return 0;
}

static int fb_screen_read_pixels(struct nemoscreen *base, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	base->node->pixman->read_pixels(base->node->pixman, base, format, pixels, x, y, width, height);

	return 0;
}

static void fb_screen_destroy(struct nemoscreen *base)
{
	struct fbscreen *screen = (struct fbscreen *)container_of(base, struct fbscreen, base);

	wl_list_remove(&base->link);

	nemoscreen_finish(base);

	free(screen);
}

static int fb_screen_switch_mode(struct nemoscreen *base, struct nemomode *mode)
{
	return 0;
}

static void fb_screen_set_gamma(struct nemoscreen *base, uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b)
{
}

static int fb_prepare_pixman_screen(struct fbscreen *screen)
{
	struct fbnode *node = screen->node;

	if (pixmanrenderer_prepare_screen(node->base.pixman, &screen->base) < 0) {
		nemolog_error("FRAMEBUFFER", "failed to prepare pixman renderer screen\n");
		return -1;
	}

	return 0;
}

static void fb_finish_pixman_screen(struct fbscreen *screen)
{
	struct fbnode *node = screen->node;

	pixmanrenderer_finish_screen(node->base.pixman, &screen->base);
}

static int fb_dispatch_frame_timeout(void *data)
{
	struct fbscreen *screen = (struct fbscreen *)data;

	return 1;
}

static int fb_create_screen(struct fbnode *node, const char *devpath)
{
	struct nemocompz *compz = node->base.compz;
	struct fbscreen *screen;
	pixman_transform_t transform;
	int shadow_width, shadow_height, width, height;
	int bytes_per_pixel;
	int fd;

	screen = (struct fbscreen *)malloc(sizeof(struct fbscreen));
	if (screen == NULL)
		return -1;
	memset(screen, 0, sizeof(struct fbscreen));

	screen->base.compz = compz;
	screen->base.node = &node->base;
	screen->base.screenid = 0;

	wl_list_init(&screen->base.mode_list);

	screen->node = node;
	screen->fbdev = strdup(devpath);

	fd = framebuffer_open(devpath, &screen->fbinfo);
	if (fd < 0) {
		nemolog_error("FRAMEBUFFER", "failed to open framebuffer device\n");
		goto err1;
	}

	screen->fbdata = mmap(NULL, screen->fbinfo.buffer_length, PROT_WRITE, MAP_SHARED, fd, 0);
	if (screen->fbdata == MAP_FAILED) {
		nemolog_error("FRAMEBUFFER", "failed to mmap framebuffer\n");
		goto err2;
	}

	screen->screen_image = pixman_image_create_bits(
			screen->fbinfo.pixel_format,
			screen->fbinfo.x_resolution,
			screen->fbinfo.y_resolution,
			screen->fbdata,
			screen->fbinfo.line_length);
	if (screen->screen_image == NULL) {
		nemolog_error("FRAMEBUFFER", "failed to create framebuffer image\n");
		goto err3;
	}

	screen->mode.flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
	screen->mode.width = screen->fbinfo.x_resolution;
	screen->mode.height = screen->fbinfo.y_resolution;
	screen->mode.refresh = screen->fbinfo.refresh_rate;
	wl_list_init(&screen->base.mode_list);
	wl_list_insert(&screen->base.mode_list, &screen->mode.link);

	screen->base.current_mode = &screen->mode;
	screen->base.subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;
	screen->base.make = "framebuffer";
	screen->base.model = screen->fbinfo.id;

	screen->base.x = 0;
	screen->base.y = 0;
	screen->base.width = screen->mode.width;
	screen->base.height = screen->mode.height;
	screen->base.mmwidth = screen->fbinfo.mmwidth;
	screen->base.mmheight = screen->fbinfo.mmheight;

	nemoscreen_prepare(&screen->base);

	width = screen->fbinfo.x_resolution;
	height = screen->fbinfo.y_resolution;
	bytes_per_pixel = screen->fbinfo.bits_per_pixel / 8;

	pixman_transform_init_identity(&transform);

	shadow_width = width;
	shadow_height = height;
	pixman_transform_rotate(&transform, NULL, 0, 0);
	pixman_transform_translate(&transform, NULL, 0, 0);

	screen->shadow_buffer = malloc(width * height * bytes_per_pixel);
	screen->shadow_image = pixman_image_create_bits(
			screen->fbinfo.pixel_format,
			shadow_width, shadow_height,
			screen->shadow_buffer,
			shadow_width * bytes_per_pixel);
	if (screen->shadow_buffer == NULL || screen->shadow_image == NULL) {
		nemolog_error("FRAMEBUFFER", "failed to create framebuffer shadow surface\n");
		goto err4;
	}

	if (fb_prepare_pixman_screen(screen) < 0)
		goto err5;

	screen->base.repaint = fb_screen_repaint;
	screen->base.repaint_frame = fb_screen_repaint_frame;
	screen->base.read_pixels = fb_screen_read_pixels;
	screen->base.destroy = fb_screen_destroy;
	screen->base.set_dpms = NULL;
	screen->base.switch_mode = fb_screen_switch_mode;
	screen->base.set_gamma = fb_screen_set_gamma;

	wl_list_insert(compz->screen_list.prev, &screen->base.link);

	nemoscreen_update_transform(&screen->base);
	nemocompz_update_scene(compz);

	nemolog_message("FRAMEBUFFER", "screen resolution %d x %d\n", screen->mode.width, screen->mode.height);

	return 0;

err5:
	pixman_image_unref(screen->shadow_image);
	free(screen->shadow_buffer);

err4:
	pixman_image_unref(screen->screen_image);

err3:
	munmap(screen->fbdata, screen->fbinfo.buffer_length);

err2:
	close(fd);

err1:
	free(screen);

	return -1;
}

static int fb_prepare_pixman(struct fbnode *node)
{
	node->base.pixman = pixmanrenderer_create(&node->base);
	if (node->base.pixman == NULL)
		return -1;

	return 0;
}

static void fb_finish_pixman(struct fbnode *node)
{
	pixmanrenderer_destroy(node->base.pixman);
}

struct fbnode *fb_create_node(struct nemocompz *compz, const char *devpath)
{
	struct fbnode *node;

	node = (struct fbnode *)malloc(sizeof(struct fbnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct fbnode));

	node->base.compz = compz;
	node->base.nodeid = 0;
	node->base.pixman = NULL;

	rendernode_prepare(&node->base);

	if (fb_prepare_pixman(node) < 0) {
		nemolog_error("FRAMEBUFFER", "failed to prepare pixman renderer\n");
		goto err1;
	}

	if (fb_create_screen(node, devpath) < 0) {
		nemolog_error("FRAMEBUFFER", "failed to create framebuffer screen\n");
		goto err2;
	}

	wl_list_insert(compz->render_list.prev, &node->base.link);

	return node;

err2:
	fb_finish_pixman(node);

err1:
	rendernode_finish(&node->base);
	free(node);

	return NULL;
}

void fb_destroy_node(struct fbnode *node)
{
	fb_finish_pixman(node);

	rendernode_finish(&node->base);

	free(node);
}
