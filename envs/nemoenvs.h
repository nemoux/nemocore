#ifndef __NEMOSHELL_ENVS_H__
#define __NEMOSHELL_ENVS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoapps.h>
#include <nemoxapp.h>
#include <nemoitem.h>

struct json_object;

struct nemoenvs;
struct nemoshell;
struct nemotimer;

struct nemoenvs {
	struct nemoshell *shell;

	struct nemoitem *apps;

	struct nemolist service_list;
	struct nemolist client_list;

	struct wl_list xserver_list;
	uint32_t xdisplay;

	struct wl_list xapp_list;
	struct wl_list xclient_list;

	struct wl_listener xserver_listener;
	int is_waiting_sigusr1;

	struct {
		int pick_taps;
	} legacy;

	struct {
		const char *path;
		const char *node;
	} xserver;

	struct {
		const char *path;
		const char *args;
	} terminal;
};

extern struct nemoenvs *nemoenvs_create(struct nemoshell *shell);
extern void nemoenvs_destroy(struct nemoenvs *envs);

extern int nemoenvs_handle_item_config(struct nemoenvs *envs, struct itemone *one);
extern int nemoenvs_handle_json_config(struct nemoenvs *envs, struct json_object *jobj);
extern int nemoenvs_handle_json_theme(struct nemoenvs *envs, struct json_object *jobj);

extern void nemoenvs_handle_terminal_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
extern void nemoenvs_handle_touch_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
extern void nemoenvs_handle_escape_key(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
extern void nemoenvs_handle_left_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data);
extern void nemoenvs_handle_right_button(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data);
extern void nemoenvs_handle_touch_event(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
