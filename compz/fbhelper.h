#ifndef	__FB_HELPER_H__
#define	__FB_HELPER_H__

#include <stdint.h>
#include <pixman.h>

struct fbscreeninfo {
	unsigned int x_resolution;
	unsigned int y_resolution;
	unsigned int mmwidth;
	unsigned int mmheight;
	unsigned int bits_per_pixel;

	size_t buffer_length;
	size_t line_length;
	char id[16];

	pixman_format_code_t pixel_format;
	unsigned int refresh_rate;
};

extern int framebuffer_query_screen_info(int fd, struct fbscreeninfo *info);
extern int framebuffer_set_screen_info(int fd, struct fbscreeninfo *info);
extern int framebuffer_open(const char *devpath, struct fbscreeninfo *info);

#endif
