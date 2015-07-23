#ifndef	__PIXMAN_HELPER_H__
#define	__PIXMAN_HELPER_H__

#include <pixman.h>

extern int pixman_save_png_file(pixman_image_t *image, const char *path);

extern pixman_image_t *pixman_load_png_file(const char *path);
extern pixman_image_t *pixman_load_jpeg_file(const char *path);

extern pixman_image_t *pixman_load_png_data(uint32_t *data, int length);
extern pixman_image_t *pixman_load_jpeg_data(uint32_t *data, int length);

#endif
