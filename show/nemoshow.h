#ifndef	__NEMOSHOW_H__
#define	__NEMOSHOW_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>
#include <nemolist.h>
#include <nemolistener.h>

#include <showone.h>
#include <showscene.h>
#include <showcanvas.h>
#include <showitem.h>
#include <showcolor.h>
#include <showexpr.h>
#include <showloop.h>
#include <showsequence.h>
#include <showease.h>
#include <showtransition.h>
#include <showmatrix.h>
#include <showpath.h>
#include <showcamera.h>

#include <nemotale.h>
#include <nemolist.h>

struct nemoshow {
	struct showone **ones;
	int nones, sones;

	struct showexpr *expr;
	struct showsymbol *stable;

	struct nemotale *tale;

	struct showone *scene;
	struct showone *camera;

	uint32_t width, height;

	struct nemolist transition_list;

	void *userdata;
};

extern void nemoshow_initialize(void);
extern void nemoshow_finalize(void);

extern struct nemoshow *nemoshow_create(void);
extern void nemoshow_destroy(struct nemoshow *show);

extern struct showone *nemoshow_search_one(struct nemoshow *show, const char *id);

extern int nemoshow_load_xml(struct nemoshow *show, const char *path);

extern void nemoshow_update_symbol(struct nemoshow *show, const char *name, double value);
extern void nemoshow_update_expression(struct nemoshow *show);
extern int nemoshow_update_one_expression(struct nemoshow *show, struct showone *one, const char *name);

extern void nemoshow_arrange_one(struct nemoshow *show);
extern void nemoshow_update_one(struct nemoshow *show);
extern void nemoshow_render_one(struct nemoshow *show);

extern int nemoshow_set_scene(struct nemoshow *show, struct showone *one);
extern void nemoshow_put_scene(struct nemoshow *show);
extern void nemoshow_dirty_scene(struct nemoshow *show);

extern int nemoshow_set_camera(struct nemoshow *show, struct showone *one);
extern void nemoshow_put_camera(struct nemoshow *show);

extern int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height);

extern int nemoshow_attach_canvas(struct nemoshow *show, struct showone *one);
extern void nemoshow_detach_canvas(struct nemoshow *show, struct showone *one);

extern int nemoshow_attach_transition(struct nemoshow *show, struct showtransition *trans);
extern void nemoshow_detach_transition(struct nemoshow *show, struct showtransition *trans);
extern void nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs);
extern int nemoshow_has_transition(struct nemoshow *show);

extern void nemoshow_dump_all(struct nemoshow *show, FILE *out);

static inline void nemoshow_set_tale(struct nemoshow *show, struct nemotale *tale)
{
	show->tale = tale;
}

static inline void nemoshow_set_userdata(struct nemoshow *show, void *data)
{
	show->userdata = data;
}

static inline void *nemoshow_get_userdata(struct nemoshow *show)
{
	return show->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
