#ifndef	__NEMOSHOW_EXPR_H__
#define	__NEMOSHOW_EXPR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>

#include <showone.h>

#define	NEMOSHOW_ATTR_EXPR_MAX		(128)

struct showexpr {
	char id[NEMOSHOW_ID_MAX];
	char name[NEMOSHOW_ATTR_NAME_MAX];

	char text[NEMOSHOW_ATTR_EXPR_MAX];
};

struct showsymbol {
	void *cc;
};

extern struct showexpr *nemoshow_expr_create(void);
extern void nemoshow_expr_destroy(struct showexpr *expr);

extern struct showsymbol *nemoshow_expr_create_symbol(void);
extern void nemoshow_expr_destroy_symbol(struct showsymbol *sym);

extern double nemoshow_expr_dispatch(struct nemoshow *show, const char *expr);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
