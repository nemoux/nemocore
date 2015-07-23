#ifndef	__WAYLAND_HELPER_H__
#define	__WAYLAND_HELPER_H__

#include <pixman.h>

extern void wayland_execute_path(int sockfd, const char *path, char *const argv[], char *const envp[]);
extern struct wl_client *wayland_execute_client(struct wl_display *display, const char *path, char *const argv[], char *const envp[], uint64_t *child);

extern void wayland_transform_point(int width, int height, enum wl_output_transform transform, int32_t scale, float sx, float sy, float *bx, float *by);
extern pixman_box32_t wayland_transform_rect(int width, int height, enum wl_output_transform transform, int32_t scale, pixman_box32_t rect);
extern void wayland_transform_region(int width, int height, enum wl_output_transform transform, int32_t scale, pixman_region32_t *src, pixman_region32_t *dest);

#endif
