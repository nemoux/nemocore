#ifndef	__NEMOSHOW_EASY_H__
#define	__NEMOSHOW_EASY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemoshow.h>

extern struct showtransition *nemoshow_transition_create_easy(struct nemoshow *show, struct showone *ease, uint32_t duration, uint32_t delay, ...);

extern void nemoshow_attach_transition_easy(struct nemoshow *show, ...);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
