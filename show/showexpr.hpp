#ifndef __NEMOSHOW_EXPR_HPP__
#define __NEMOSHOW_EXPR_HPP__

#include <exprtk.hpp>

typedef exprtk::symbol_table<double> symbol_table_t;
typedef exprtk::expression<double> expression_t;
typedef exprtk::parser<double> parser_t;

struct _showsymbol {
	symbol_table_t symbol_table;
};

#endif
