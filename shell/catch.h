#ifndef	__NEMO_SHELL_CATCH_H__
#define	__NEMO_SHELL_CATCH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoshell;
struct nemoview;

extern void nemoshell_catch_view(struct nemoview *view);
extern void nemoshell_uncatch_view(struct nemoview *view);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
