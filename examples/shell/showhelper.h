#ifndef __MINISHELL_SHOW_HELPER_H__
#define __MINISHELL_SHOW_HELPER_H__

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

struct nemocompz;
struct nemoactor;

struct showcontext {
	struct nemocompz *compz;

	struct nemoactor *actor;

	struct nemotale *tale;

	int32_t width, height;
};

#define NEMOSHOW_AT(show, at)			(((struct showcontext *)nemoshow_get_context(show))->at)

extern struct nemoshow *nemoshow_create_on_actor(struct nemocompz *compz, int32_t width, int32_t height, nemotale_dispatch_event_t dispatch);
extern void nemoshow_destroy_on_actor(struct nemoshow *show);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
