#ifndef __NEMOSHELL_PACK_H__
#define __NEMOSHELL_PACK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include <showhelper.h>

#define NEMOPACK_TIMEOUT			(1000)

struct nemoshell;
struct nemoview;
struct nemotimer;

struct nemopack {
	struct nemoshell *shell;

	struct nemoview *view;
	struct shellbin *bin;

	struct nemolistener destroy_listener;

	struct wl_listener view_destroy_listener;
	struct wl_listener bin_resize_listener;

	struct nemoshow *show;

	struct showone *scene;

	struct showone *back;
	struct showone *canvas;
	
	struct nemotimer *timer;

	uint32_t timeout;
};

extern void nemopack_prepare_envs(void);
extern void nemopack_finish_envs(void);

extern struct nemopack *nemopack_create(struct nemoshell *shell, struct nemoview *view, uint32_t timeout);
extern void nemopack_destroy(struct nemopack *pack);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
