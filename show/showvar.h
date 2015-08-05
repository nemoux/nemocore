#ifndef __NEMOSHOW_VAR_H__
#define __NEMOSHOW_VAR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotale.h>
#include <talegl.h>

#include <showone.h>

struct showref {
	struct nemoattr *attr;
	char name[NEMOSHOW_ATTR_NAME_MAX];
	int type;
};

struct showvar {
	struct showone base;

	struct showref **refs;
	int nrefs, srefs;
};

#define NEMOSHOW_VAR(one)						((struct showvar *)container_of(one, struct showvar, base))
#define	NEMOSHOW_VAR_AT(one, at)		(NEMOSHOW_VAR(one)->at)

extern struct showone *nemoshow_var_create(void);
extern void nemoshow_var_destroy(struct showone *one);

extern struct showref *nemoshow_var_create_ref(struct showone *one, const char *name, int type);
extern void nemoshow_var_destroy_ref(struct showref *ref);

extern int nemoshow_var_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_var_update(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
