#ifndef	__NEMO_LAYER_H__
#define	__NEMO_LAYER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemolayer {
	struct wl_list view_list;
	struct wl_list link;
};

extern void nemolayer_prepare(struct nemolayer *layer, struct wl_list *below);
extern void nemolayer_finish(struct nemolayer *layer);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
