#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <linux/fb.h>

#include <fbhelper.h>
#include <nemomisc.h>

static pixman_format_code_t framebuffer_calculate_pixman_format(struct fb_var_screeninfo *vinfo, struct fb_fix_screeninfo *finfo)
{
	int type;

	if (finfo->type != FB_TYPE_PACKED_PIXELS)
		return 0;

	switch (finfo->visual) {
		case FB_VISUAL_TRUECOLOR:
		case FB_VISUAL_DIRECTCOLOR:
			if (vinfo->grayscale != 0)
				return 0;
			break;

		default:
			return 0;
	}

	if (vinfo->red.msb_right != 0 ||
			vinfo->green.msb_right != 0 ||
			vinfo->blue.msb_right != 0)
		return 0;

	type = PIXMAN_TYPE_OTHER;

	if ((vinfo->transp.offset >= vinfo->red.offset ||
				vinfo->transp.length == 0) &&
			vinfo->red.offset >= vinfo->green.offset &&
			vinfo->green.offset >= vinfo->blue.offset)
		type = PIXMAN_TYPE_ARGB;
	else if (vinfo->red.offset >= vinfo->green.offset &&
			vinfo->green.offset >= vinfo->blue.offset &&
			vinfo->blue.offset >= vinfo->transp.offset)
		type = PIXMAN_TYPE_RGBA;

	if (type == PIXMAN_TYPE_OTHER)
		return 0;

	return PIXMAN_FORMAT(vinfo->bits_per_pixel,
			type,
			vinfo->transp.length,
			vinfo->red.length,
			vinfo->green.length,
			vinfo->blue.length);
}

static int framebuffer_calculate_refresh_rate(struct fb_var_screeninfo *vinfo)
{
	uint64_t quot;

	quot = (vinfo->upper_margin + vinfo->lower_margin + vinfo->yres);
	quot *= (vinfo->left_margin + vinfo->right_margin + vinfo->xres);
	quot *= vinfo->pixclock;

	if (quot > 0) {
		uint64_t refresh_rate;

		refresh_rate = 1000000000000000LLU / quot;
		if (refresh_rate > 200000)
			refresh_rate = 200000;

		return refresh_rate;
	}

	return 60 * 1000;
}

int framebuffer_query_screen_info(int fd, struct fbscreeninfo *info)
{
	struct fb_var_screeninfo varinfo;
	struct fb_fix_screeninfo fixinfo;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fixinfo) < 0 ||
			ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	info->x_resolution = varinfo.xres;
	info->y_resolution = varinfo.yres;
	info->mmwidth = varinfo.width;
	info->mmheight = varinfo.height;
	info->bits_per_pixel = varinfo.bits_per_pixel;

	info->buffer_length = fixinfo.smem_len > 0 ? fixinfo.smem_len : fixinfo.line_length * varinfo.yres;
	info->line_length = fixinfo.line_length;
	strncpy(info->id, fixinfo.id, sizeof(info->id) / sizeof(info->id[0]));
	info->id[MIN(strlen(fixinfo.id), sizeof(info->id) / sizeof(info->id[0]))] = '\0';

	info->pixel_format = framebuffer_calculate_pixman_format(&varinfo, &fixinfo);
	info->refresh_rate = framebuffer_calculate_refresh_rate(&varinfo);

	if (info->pixel_format == 0) {
		return -1;
	}

	return 1;
}

int framebuffer_set_screen_info(int fd, struct fbscreeninfo *info)
{
	struct fb_var_screeninfo varinfo;

	if (ioctl(fd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	varinfo.xres = info->x_resolution;
	varinfo.yres = info->y_resolution;
	varinfo.width = info->mmwidth;
	varinfo.height = info->mmheight;
	varinfo.bits_per_pixel = info->bits_per_pixel;

	varinfo.grayscale = 0;
	varinfo.transp.offset = 24;
	varinfo.transp.length = 0;
	varinfo.transp.msb_right = 0;
	varinfo.red.offset = 16;
	varinfo.red.length = 8;
	varinfo.red.msb_right = 0;
	varinfo.green.offset = 8;
	varinfo.green.length = 8;
	varinfo.green.msb_right = 0;
	varinfo.blue.offset = 0;
	varinfo.blue.length = 8;
	varinfo.blue.msb_right = 0;

	if (ioctl(fd, FBIOPUT_VSCREENINFO, &varinfo) < 0) {
		return -1;
	}

	return 1;
}

int framebuffer_open(const char *devpath, struct fbscreeninfo *info)
{
	int fd;

	fd = open(devpath, O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (fd < 0)
		return -1;

	if (framebuffer_query_screen_info(fd, info) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}
