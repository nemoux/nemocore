#ifndef __MINISHELL_H__
#define __MINISHELL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <showhelper.h>

#include <nemolist.h>
#include <nemolistener.h>

struct minishell {
	struct nemoshell *shell;

	struct nemoshow *show;

	struct nemolist grab_list;

	uint32_t serial;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
