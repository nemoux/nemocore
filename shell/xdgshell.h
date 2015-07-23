#ifndef	__NEMO_XDG_SHELL_H__
#define	__NEMO_XDG_SHELL_H__

extern int xdgshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

extern int xdgshell_is_xdg_surface(struct shellbin *bin);
extern int xdgshell_is_xdg_popup(struct shellbin *bin);

#endif
