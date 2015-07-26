#ifndef	__NEMOSHOW_H__
#define	__NEMOSHOW_H__

#include <stdint.h>

#include <nemoattr.h>
#include <nemolist.h>
#include <nemolistener.h>

#include <showone.h>
#include <showscene.h>

struct nemoshow {
	struct nemolist ones;
};

extern struct nemoshow *nemoshow_create(void);
extern void nemoshow_destroy(struct nemoshow *show);

extern int nemoshow_load_xml(struct nemoshow *show, const char *path);

extern void nemoshow_dump_all(struct nemoshow *show, FILE *out);

#endif
