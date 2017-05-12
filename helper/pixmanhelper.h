#ifndef	__PIXMAN_HELPER_H__
#define	__PIXMAN_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

extern int pixman_save_png_file(pixman_image_t *image, const char *path);

extern pixman_image_t *pixman_load_png_file(const char *path);
extern pixman_image_t *pixman_load_png_data(uint32_t *data, int length);
extern pixman_image_t *pixman_load_jpeg_file(const char *path);
extern pixman_image_t *pixman_load_jpeg_data(uint32_t *data, int length);
extern pixman_image_t *pixman_load_svg_file(const char *path);
extern pixman_image_t *pixman_load_svg_data(uint32_t *data, int length);

extern pixman_image_t *pixman_load_image(const char *filepath, int32_t width, int32_t height);

extern pixman_image_t *pixman_resize_image(pixman_image_t *src, int32_t width, int32_t height);
extern pixman_image_t *pixman_clone_image(pixman_image_t *src);

extern int pixman_copy_image(pixman_image_t *dst, pixman_image_t *src);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
