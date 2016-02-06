#ifndef __NEMOUX_MIRO_TAP_H__
#define __NEMOUX_MIRO_TAP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include <showhelper.h>

typedef enum {
	MIROBACK_TAP_NORMAL_STATE = 0,
	MIROBACK_TAP_COLLISION_STATE = 1,
	MIROBACK_TAP_LAST_STATE
} MiroBackTapState;

struct mirotap {
	struct miroback *miro;

	struct nemolist link;

	struct nemosignal destroy_signal;

	struct nemotimer *timer;

	struct showone *one0;
	struct showone *one1;
	struct showone *one2;
	struct showone *one3;
	struct showone *one4;

	struct showone *oner;

	struct showone *blur;

	uint32_t state;

	double x, y;
};

extern struct mirotap *miroback_tap_create(struct miroback *mini);
extern void miroback_tap_destroy(struct mirotap *tap);

extern int miroback_tap_down(struct miroback *mini, struct mirotap *tap, double x, double y);
extern int miroback_tap_motion(struct miroback *mini, struct mirotap *tap, double x, double y);
extern int miroback_tap_up(struct miroback *mini, struct mirotap *tap, double x, double y);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
