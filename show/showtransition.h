#ifndef	__NEMOSHOW_TRANSITION_H__
#define	__NEMOSHOW_TRANSITION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showsequence.h>
#include <showease.h>

struct showtransition {
	struct showone **sequences;
	int nsequences, ssequences;

	struct showease *ease;

	uint32_t duration;
	uint32_t delay;

	uint32_t stime;
	uint32_t etime;
};

extern struct showtransition *nemoshow_transition_create(struct showone *ease, uint32_t duration, uint32_t delay);
extern void nemoshow_transition_destroy(struct showtransition *trans);

extern void nemoshow_transition_attach_sequence(struct showtransition *trans, struct showone *sequence);

extern int nemoshow_transition_dispatch(struct showtransition *trans, uint32_t time);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
