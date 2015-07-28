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
#include <showshape.h>
#include <showcolor.h>
#include <showexpr.h>
#include <showloop.h>
#include <showsequence.h>
#include <showease.h>
#include <showtransition.h>

struct nemoshow {
	struct showone **ones;
	int nones, sones;

	struct showexpr *expr;
	struct showsymbol *stable;
};

extern struct nemoshow *nemoshow_create(void);
extern void nemoshow_destroy(struct nemoshow *show);

extern void nemoshow_sort_one(struct nemoshow *show);
extern struct showone *nemoshow_search_one(struct nemoshow *show, const char *id);

extern int nemoshow_load_xml(struct nemoshow *show, const char *path);

extern void nemoshow_update_symbol(struct nemoshow *show, const char *name, double value);
extern void nemoshow_update_expression(struct nemoshow *show);
extern int nemoshow_update_one_expression(struct nemoshow *show, struct showone *one, const char *name);

extern void nemoshow_arrange_one(struct nemoshow *show);

extern void nemoshow_dump_all(struct nemoshow *show, FILE *out);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
