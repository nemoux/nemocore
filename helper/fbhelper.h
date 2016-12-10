#ifndef	__FB_HELPER_H__
#define	__FB_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
