#ifndef	__NEMO_PICKER_H__
#define	__NEMO_PICKER_H__

struct nemocompz;
struct nemoview;

extern struct nemoview *nemocompz_pick_view(struct nemocompz *compz, float x, float y, float *sx, float *sy);
extern struct nemoview *nemocompz_pick_canvas(struct nemocompz *compz, float x, float y, float *sx, float *sy);

#endif
