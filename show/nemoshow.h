#ifndef	__NEMOSHOW_H__
#define	__NEMOSHOW_H__

#include <stdint.h>

#include <nemoattr.h>
#include <nemolist.h>
#include <nemolistener.h>

#include <showone.h>
#include <showscene.h>
#include <showcanvas.h>

struct nemoshow {
	struct showone **ones;
	int nones, sones;
};

extern struct nemoshow *nemoshow_create(void);
extern void nemoshow_destroy(struct nemoshow *show);

extern void nemoshow_update_one(struct nemoshow *show);
extern struct showone *nemoshow_search_one(struct nemoshow *show, const char *id);

extern int nemoshow_load_xml(struct nemoshow *show, const char *path);

extern void nemoshow_dump_all(struct nemoshow *show, FILE *out);

#endif
