#ifndef __NEMOSHOW_EXPR_HPP__
#define __NEMOSHOW_EXPR_HPP__

#include <exprtk.hpp>

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;

typedef struct _showexpr {
	expression_t expression;
} showexpr_t;

typedef struct _showsymtable {
	symbol_table_t symbol_table;
} showsymtable_t;

#define	NEMOSHOW_EXPR_CC(base, name)					(((showexpr_t *)((base)->cc))->name)
#define	NEMOSHOW_SYMTABLE_CC(base, name)			(((showsymtable_t *)((base)->cc))->name)

#endif
