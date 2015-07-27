#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showexpr.h>
#include <showexpr.hpp>
#include <nemoshow.h>
#include <nemomisc.h>

struct showexpr *nemoshow_expr_create(void)
{
	struct showexpr *expr;

	expr = (struct showexpr *)malloc(sizeof(struct showexpr));
	if (expr == NULL)
		return NULL;
	memset(expr, 0, sizeof(struct showexpr));

	return expr;
}

void nemoshow_expr_destroy(struct showexpr *expr)
{
	free(expr);
}

struct showsymbol *nemoshow_expr_create_symbol(void)
{
	struct showsymbol *sym;

	sym = (struct showsymbol *)malloc(sizeof(struct showsymbol));
	if (sym == NULL)
		return NULL;
	memset(sym, 0, sizeof(struct showsymbol));

	sym->cc = (struct _showsymbol *)malloc(sizeof(struct _showsymbol));
	if (sym->cc == NULL)
		goto err1;

	return sym;

err1:
	free(sym);

	return NULL;
}

void nemoshow_expr_destroy_symbol(struct showsymbol *sym)
{
	free(sym->cc);
	free(sym);
}

double nemoshow_expr_dispatch(struct nemoshow *show, const char *expr)
{
}
