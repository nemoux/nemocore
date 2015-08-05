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

void nemoshow_expr_add_symbol_table(struct showexpr *expr, struct showsymtable *stable)
{
	NEMOSHOW_EXPR_CC(expr, expression).register_symbol_table(NEMOSHOW_SYMTABLE_CC(stable, symbol_table));
}

double nemoshow_expr_dispatch_expression(struct showexpr *expr, const char *text)
{
	parser_t parser;
	parser.compile(text, NEMOSHOW_EXPR_CC(expr, expression));

	return NEMOSHOW_EXPR_CC(expr, expression).value();
}

struct showsymtable *nemoshow_expr_create_symbol_table(void)
{
	struct showsymtable *stable;

	stable = (struct showsymtable *)malloc(sizeof(struct showsymtable));
	if (stable == NULL)
		return NULL;
	memset(stable, 0, sizeof(struct showsymtable));

	stable->cc = new showsymtable_t;

	stable->symbols = (struct showsymbol **)malloc(sizeof(struct showsymbol *) * 8);
	stable->nsymbols = 0;
	stable->ssymbols = 8;

	NEMOSHOW_SYMTABLE_CC(stable, symbol_table).add_constants();

	return stable;
}

void nemoshow_expr_destroy_symbol_table(struct showsymtable *stable)
{
	delete static_cast<showsymtable_t *>(stable->cc);

	free(stable->symbols);
	free(stable);
}

struct showsymbol *nemoshow_expr_create_symbol(void)
{
	struct showsymbol *symbol;

	symbol = (struct showsymbol *)malloc(sizeof(struct showsymbol));
	if (symbol == NULL)
		return NULL;
	memset(symbol, 0, sizeof(struct showsymbol));

	return symbol;
}

void nemoshow_expr_destroy_symbol(struct showsymbol *symbol)
{
	free(symbol);
}

int nemoshow_expr_add_symbol(struct showsymtable *stable, const char *name, double value)
{
	struct showsymbol *symbol;
	int i;

	for (i = 0; i < stable->nsymbols; i++) {
		symbol = stable->symbols[i];

		if (strcmp(symbol->name, name) == 0) {
			symbol->value = value;

			return 1;
		}
	}

	symbol = nemoshow_expr_create_symbol();
	if (symbol == NULL)
		return 0;

	strcpy(symbol->name, name);
	symbol->value = value;

	NEMOSHOW_SYMTABLE_CC(stable, symbol_table).add_variable(name, symbol->value);

	NEMOBOX_APPEND(stable->symbols, stable->ssymbols, stable->nsymbols, symbol);

	return 1;
}
