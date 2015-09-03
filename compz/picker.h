#ifndef	__NEMO_PICKER_H__
#define	__NEMO_PICKER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemocompz;
struct nemoview;

extern struct nemoview *nemocompz_pick_view(struct nemocompz *compz, float x, float y, float *sx, float *sy);
extern struct nemoview *nemocompz_pick_view_below(struct nemocompz *compz, float x, float y, float *sx, float *sy, struct nemoview *below);
extern struct nemoview *nemocompz_pick_canvas(struct nemocompz *compz, float x, float y, float *sx, float *sy);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
