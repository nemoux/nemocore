#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <picker.h>
#include <compz.h>
#include <view.h>
#include <content.h>
#include <scope.h>
#include <nemomisc.h>

struct nemoview *nemocompz_pick_view(struct nemocompz *compz, float x, float y, float *sx, float *sy, uint32_t state)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

#define	NEMOCOMPZ_PICK_VIEW(v, x, y, sx, sy)	\
	nemoview_transform_from_global(v, x, y, sx, sy);	\
	if (nemoview_has_state(v, NEMOVIEW_SCOPE_STATE) != 0) {	\
		if (nemoscope_pick(v->scope, *sx, *sy)) return v;	\
	} else if (v->content->pick != NULL) {	\
		if (v->content->pick(v->content, *sx, *sy)) return v;	\
	} else {	\
		if (pixman_region32_contains_point(&v->content->input, *sx, *sy, NULL)) return v;	\
	}

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (nemoview_has_state_all(child, state) != 0) {
						NEMOCOMPZ_PICK_VIEW(child, x, y, sx, sy);
					}
				}
			}

			if (nemoview_has_state_all(view, state) != 0) {
				NEMOCOMPZ_PICK_VIEW(view, x, y, sx, sy);
			}
		}
	}

	return NULL;
}

struct nemoview *nemocompz_pick_view_below(struct nemocompz *compz, float x, float y, float *sx, float *sy, struct nemoview *below, uint32_t state)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;
	int belowed = 0;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (belowed == 0) {
				if (view == below) {
					belowed = 1;
				} else {
					if (!wl_list_empty(&view->children_list)) {
						wl_list_for_each(child, &view->children_list, children_link) {
							if (child == below) {
								belowed = 1;
								break;
							}
						}
					}
				}
			} else {
				if (!wl_list_empty(&view->children_list)) {
					wl_list_for_each(child, &view->children_list, children_link) {
						if (nemoview_has_state_all(child, state) != 0) {
							NEMOCOMPZ_PICK_VIEW(child, x, y, sx, sy);
						}
					}
				}

				if (nemoview_has_state_all(view, state) != 0) {
					NEMOCOMPZ_PICK_VIEW(view, x, y, sx, sy);
				}
			}
		}
	}

	return NULL;
}
