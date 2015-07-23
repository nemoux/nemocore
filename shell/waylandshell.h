#ifndef	__NEMO_WAYLAND_SHELL_H__
#define	__NEMO_WAYLAND_SHELL_H__

struct shellbin;

extern int waylandshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

extern int waylandshell_is_shell_surface(struct shellbin *bin);

#endif
