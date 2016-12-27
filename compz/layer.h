#ifndef	__NEMO_LAYER_H__
#define	__NEMO_LAYER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemocompz;

struct nemolayer {
	struct nemocompz *compz;

	char *name;

	struct wl_list view_list;
	struct wl_list link;
};

extern struct nemolayer *nemolayer_create(struct nemocompz *compz, const char *name);
extern void nemolayer_destroy(struct nemolayer *layer);

extern void nemolayer_attach_below(struct nemolayer *layer, struct nemolayer *below);
extern void nemolayer_attach_above(struct nemolayer *layer, struct nemolayer *above);
extern void nemolayer_detach(struct nemolayer *layer);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
