#ifndef	__MINISHELL_TALE_HELPER_H__
#define	__MINISHELL_TALE_HELPER_H__

#include <nemotale.h>
#include <talenode.h>
#include <talepixman.h>
#include <talegl.h>
#include <taleevent.h>
#include <talegrab.h>
#include <taletransition.h>
#include <talegesture.h>
#include <talemisc.h>
#include <pathshape.h>
#include <pathstyle.h>

extern void nemotale_attach_actor(struct nemotale *tale, struct nemoactor *actor, nemotale_dispatch_event_t dispatch);

#endif
