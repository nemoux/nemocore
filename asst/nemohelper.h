#ifndef __NEMO_HELPER_H__
#define __NEMO_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
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
extern int os_epoll_add_fd(int efd, int fd, uint32_t events, void *data);
extern int os_epoll_del_fd(int efd, int fd);
extern int os_epoll_set_fd(int efd, int fd, uint32_t events, void *data);
extern int os_create_anonymous_file(off_t size);

extern int os_exist_path(const char *path);
extern pid_t os_execute_path(const char *path, char *const argv[]);
extern int os_load_path(const char *path, char **buffer, int *size);
extern int os_save_path(const char *path, char *buffer, int size);

extern int os_append_path(char *path, const char *name);
extern const char *os_get_file_extension(const char *name);
extern int os_has_file_extension(const char *name, ...);
extern char *os_get_file_path(const char *name);

extern int os_set_nonblocking_mode(int fd);

extern int udp_create_socket(const char *ip, int port);
extern int udp_send_to(int soc, const char *ip, int port, const char *msg, int size);
extern int udp_recv_from(int soc, char *ip, int *port, char *msg, int size);

extern double cubicbezier_point(double t, double p0, double p1, double p2, double p3);
extern double cubicbezier_length(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int steps);

#define	COLOR_TO_UINT32(b)	\
	((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16) | ((uint32_t)b[1] << 8) | ((uint32_t)b[0] << 0)

#define	COLOR_UINT32_A(u)	\
	(uint8_t)((u >> 24) & 0xff)
#define	COLOR_UINT32_R(u)	\
	(uint8_t)((u >> 16) & 0xff)
#define	COLOR_UINT32_G(u)	\
	(uint8_t)((u >> 8) & 0xff)
#define	COLOR_UINT32_B(u)	\
	(uint8_t)((u >> 0) & 0xff)

#define	COLOR_DOUBLE_A(u)	\
	(double)COLOR_UINT32_A(u) / (double)255
#define	COLOR_DOUBLE_R(u)	\
	(double)COLOR_UINT32_R(u) / (double)255
#define	COLOR_DOUBLE_G(u)	\
	(double)COLOR_UINT32_G(u) / (double)255
#define	COLOR_DOUBLE_B(u)	\
	(double)COLOR_UINT32_B(u) / (double)255

extern uint32_t color_parse(const char *value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
