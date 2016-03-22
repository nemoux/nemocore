#ifndef __NEMO_HELPER_H__
#define __NEMO_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

extern int pixman_save_png_file(pixman_image_t *image, const char *path);

extern pixman_image_t *pixman_load_png_file(const char *path);
extern pixman_image_t *pixman_load_jpeg_file(const char *path);

extern pixman_image_t *pixman_load_png_data(uint32_t *data, int length);
extern pixman_image_t *pixman_load_jpeg_data(uint32_t *data, int length);

extern pixman_image_t *pixman_load_image(const char *filepath, int32_t width, int32_t height);

extern int pixman_copy_image(pixman_image_t *dst, pixman_image_t *src);

extern int os_socketpair_cloexec(int domain, int type, int protocol, int *sv);
extern int os_epoll_create_cloexec(void);
extern int os_create_anonymous_file(off_t size);

extern pid_t os_execute_path(const char *path, char *const argv[]);
extern int os_load_path(const char *path, char **buffer, int *size);
extern int os_save_path(const char *path, char *buffer, int size);

extern int os_append_path(char *path, const char *name);
extern const char *os_get_file_extension(const char *name);
extern char *os_get_file_path(const char *name);

extern int os_set_nonblocking_mode(int fd);

extern double cubicbezier_point(double t, double p0, double p1, double p2, double p3);
extern double cubicbezier_length(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int steps);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
