#ifndef	__NEMO_LAYER_H__
#define	__NEMO_LAYER_H__

struct nemolayer {
	struct wl_list view_list;
	struct wl_list link;
};

extern void nemolayer_prepare(struct nemolayer *layer, struct wl_list *below);
extern void nemolayer_finish(struct nemolayer *layer);

#endif
