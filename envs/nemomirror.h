#ifndef __NEMOSHELL_MIRROR_H__
#define __NEMOSHELL_MIRROR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolistener.h>

struct nemoshell;
struct nemoview;
struct shellscreen;
struct nemoshow;
struct showone;

struct nemomirror {
	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct nemoshell *shell;
	struct nemoview *view;

	struct wl_listener view_damage_listener;
	struct wl_listener view_destroy_listener;

	struct wl_listener destroy_listener;
};

extern struct nemomirror *nemomirror_create(struct nemoshell *shell, int32_t x, int32_t y, int32_t width, int32_t height, const char *layer);
extern void nemomirror_destroy(struct nemomirror *mirror);

extern int nemomirror_set_view(struct nemomirror *mirror, struct nemoview *view);

extern int nemomirror_check_screen(struct nemomirror *mirror, struct shellscreen *screen);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
