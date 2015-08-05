#ifndef	__NEMOSHOW_EXPR_H__
#define	__NEMOSHOW_EXPR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>

#include <showone.h>

#define	NEMOSHOW_EXPR_SYMBOL_NAME_MAX		(32)

struct showexpr {
	void *cc;
};

struct showsymbol {
	char name[NEMOSHOW_EXPR_SYMBOL_NAME_MAX];

	double value;
};

struct showsymtable {
	struct showsymbol **symbols;
	int nsymbols, ssymbols;

	void *cc;
};

extern struct showexpr *nemoshow_expr_create(void);
extern void nemoshow_expr_destroy(struct showexpr *expr);

extern void nemoshow_expr_add_symbol_table(struct showexpr *expr, struct showsymtable *stable);
extern double nemoshow_expr_dispatch_expression(struct showexpr *expr, const char *text);

extern struct showsymtable *nemoshow_expr_create_symbol_table(void);
extern void nemoshow_expr_destroy_symbol_table(struct showsymtable *stable);

extern struct showsymbol *nemoshow_expr_create_symbol(void);
extern void nemoshow_expr_destroy_symbol(struct showsymbol *symbol);

extern int nemoshow_expr_add_symbol(struct showsymtable *stable, const char *name, double value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
