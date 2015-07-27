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

	expr->cc = new showexpr_t;

	return expr;
}

void nemoshow_expr_destroy(struct showexpr *expr)
{
	delete static_cast<showexpr_t *>(expr->cc);

	free(expr);
}

void nemoshow_expr_add_symbol_table(struct showexpr *expr, struct showsymbol *stable)
{
	NEMOSHOW_EXPR_CC(expr, expression).register_symbol_table(NEMOSHOW_SYMBOL_CC(stable, symbol_table));
}

double nemoshow_expr_dispatch_expression(struct showexpr *expr, const char *text)
{
	parser_t parser;
	parser.compile(text, NEMOSHOW_EXPR_CC(expr, expression));

	return NEMOSHOW_EXPR_CC(expr, expression).value();
}

struct showsymbol *nemoshow_expr_create_symbol(void)
{
	struct showsymbol *stable;

	stable = (struct showsymbol *)malloc(sizeof(struct showsymbol));
	if (stable == NULL)
		return NULL;
	memset(stable, 0, sizeof(struct showsymbol));

	stable->cc = new showsymbol_t;

	NEMOSHOW_SYMBOL_CC(stable, symbol_table).add_constants();

	return stable;
}

void nemoshow_expr_destroy_symbol(struct showsymbol *stable)
{
	delete static_cast<showsymbol_t *>(stable->cc);

	free(stable);
}

int nemoshow_expr_add_symbol(struct showsymbol *stable, const char *name, double value)
{
	int i;
	int r = 0;

	for (i = 0; i < NEMOSHOW_EXPR_SYMBOL_MAX; i++) {
		if (strcmp(stable->names[i], name) == 0) {
			break;
		} else if (stable->names[i][0] == '\0') {
			strcpy(stable->names[i], name);

			break;
		}
	}

	if (i < NEMOSHOW_EXPR_SYMBOL_MAX) {
		stable->vars[i] = value;

		NEMOSHOW_SYMBOL_CC(stable, symbol_table).add_variable(name, stable->vars[i]);

		r = 1;
	}

	return r;
}
