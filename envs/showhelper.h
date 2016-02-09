#ifndef __NEMOUX_SHOW_HELPER_H__
#define __NEMOUX_SHOW_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>
#include <taleevent.h>
#include <talegrab.h>
#include <talegesture.h>

#include <nemoshow.h>

struct nemoshell;
struct nemocompz;
struct nemoactor;
struct nemotimer;

struct showcontext {
	struct nemoshell *shell;
	struct nemocompz *compz;
	struct nemoactor *actor;
	struct nemotimer *timer;

	struct nemotale *tale;

	struct nemoshow *show;

	int32_t width, height;
};

#define NEMOSHOW_AT(show, at)			(((struct showcontext *)nemoshow_get_context(show))->at)

extern struct nemoshow *nemoshow_create_actor(struct nemoshell *shell, int32_t width, int32_t height, nemotale_dispatch_event_t dispatch);
extern void nemoshow_destroy_actor(struct nemoshow *show);

extern void nemoshow_destroy_actor_on_idle(struct nemoshow *show);

extern void nemoshow_revoke_actor(struct nemoshow *show);

extern void nemoshow_dispatch_frame(struct nemoshow *show);
extern void nemoshow_dispatch_feedback(struct nemoshow *show);
extern void nemoshow_terminate_feedback(struct nemoshow *show);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
