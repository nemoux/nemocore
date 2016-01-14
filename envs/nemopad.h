#ifndef __NEMOSHELL_PAD_H__
#define __NEMOSHELL_PAD_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <showhelper.h>

#define NEMOPAD_WIDTH						(964)
#define NEMOPAD_HEIGHT					(322)
#define NEMOPAD_MINIMUM_WIDTH		(NEMOPAD_WIDTH / 2)
#define NEMOPAD_MINIMUM_HEIGHT	(NEMOPAD_HEIGHT / 2)

#define	NEMOPAD_TIMEOUT					(700)

#define NEMOPAD_KEYS_MAX				(80)

typedef enum {
	NEMOPAD_DESTROY_STATE = (1 << 0),
	NEMOPAD_ACTIVE_STATE = (1 << 1),
} NemoShellPadState;

struct nemopad {
	struct nemoshell *shell;

	struct nemosignal destroy_signal;

	struct nemotimer *timer;

	struct nemokeypad *keypad;

	struct nemoshow *show;

	struct showone *scene;

	struct showone *back;
	struct showone *canvas;

	struct showone *borders[NEMOPAD_KEYS_MAX];
	struct showone *keys[NEMOPAD_KEYS_MAX];

	uint32_t state;

	int is_upper_case;
	int is_shift_case;

	int is_pickable;
	int is_visible;

	struct wl_listener actor_endgrab_listener;

	uint32_t width;
	uint32_t height;
	uint32_t minwidth;
	uint32_t minheight;
	uint32_t timeout;
};

extern void nemopad_prepare_envs(void);
extern void nemopad_finish_envs(void);

extern struct nemopad *nemopad_create(struct nemoshell *shell, uint32_t width, uint32_t height, uint32_t minwidth, uint32_t minheight, uint32_t timeout);
extern void nemopad_destroy(struct nemopad *pad);

extern int nemopad_activate(struct nemopad *pad, double x, double y, double r);
extern void nemopad_deactivate(struct nemopad *pad);

extern void nemopad_set_focus(struct nemopad *pad, struct nemoview *view);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
