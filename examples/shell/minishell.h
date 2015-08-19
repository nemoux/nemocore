#ifndef __MINISHELL_H__
#define __MINISHELL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>
#include <showhelper.h>

struct minishell {
	struct nemoshell *shell;

	struct nemoshow *show;
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
