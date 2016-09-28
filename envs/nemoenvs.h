#ifndef __NEMOSHELL_ENVS_H__
#define __NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoapps.h>
#include <nemoxapp.h>
#include <nemomsg.h>

#include <nemobox.h>
#include <nemoitem.h>

struct nemoenvs;

typedef int (*nemoenvs_callback_t)(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);

struct nemoshell;

struct envscallback {
	nemoenvs_callback_t callback;
	void *data;

	struct nemolist link;
};

struct nemoenvs {
	struct nemoshell *shell;

	struct nemoitem *configs;

	struct nemolist app_list;
	struct nemolist client_list;

	struct nemolist callback_list;

	struct nemomsg *msg;
	struct nemomonitor *monitor;

	char *name;
	char args[512];

	struct wl_list xserver_list;
	uint32_t xdisplay;

	struct wl_list xapp_list;
	struct wl_list xclient_list;

	struct wl_listener xserver_listener;
	int is_waiting_sigusr1;

	struct {
		int pick_taps;
	} legacy;
};

extern struct nemoenvs *nemoenvs_create(struct nemoshell *shell);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_listen(struct nemoenvs *envs, const char *ip, int port);
extern int nemoenvs_send(struct nemoenvs *envs, const char *name, const char *fmt, ...);
extern int nemoenvs_reply(struct nemoenvs *envs, const char *fmt, ...);

extern void nemoenvs_set_name(struct nemoenvs *envs, const char *fmt, ...);
extern void nemoenvs_set_args(struct nemoenvs *envs, char *args[], int argc);

extern int nemoenvs_set_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data);
extern int nemoenvs_put_callback(struct nemoenvs *envs, nemoenvs_callback_t callback, void *data);

extern int nemoenvs_dispatch(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one);

extern int nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath);
extern int nemoenvs_save_configs(struct nemoenvs *envs, const char *configpath);

extern int nemoenvs_dispatch_system_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);
extern int nemoenvs_dispatch_device_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);
extern int nemoenvs_dispatch_config_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);
extern int nemoenvs_dispatch_link_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);
extern int nemoenvs_dispatch_envs_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);
extern int nemoenvs_dispatch_client_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data);

extern void nemoenvs_handle_terminal_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
extern void nemoenvs_handle_touch_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
extern void nemoenvs_handle_escape_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
extern void nemoenvs_handle_left_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data);
extern void nemoenvs_handle_right_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data);
extern void nemoenvs_handle_touch_event(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data);

static inline const char *nemoenvs_get_name(struct nemoenvs *envs)
{
	return envs->name;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
