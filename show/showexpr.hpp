#ifndef __NEMOSHOW_EXPR_HPP__
#define __NEMOSHOW_EXPR_HPP__

#include <exprtk.hpp>

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;

typedef struct _showexpr {
	expression_t expression;
} showexpr_t;

typedef struct _showsymbol {
	symbol_table_t symbol_table;
} showsymbol_t;

#define	NEMOSHOW_EXPR_CC(base, name)				(((showexpr_t *)((base)->cc))->name)
#define	NEMOSHOW_SYMBOL_CC(base, name)			(((showsymbol_t *)((base)->cc))->name)

#endif
