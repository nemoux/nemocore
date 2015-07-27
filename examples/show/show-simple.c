#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct nemoshow *show;
	struct showexpr *expr;
	struct showone *one;

	show = nemoshow_create();
	nemoshow_load_xml(show, argv[1]);
	nemoshow_update_one(show);

	nemoshow_dump_all(show, stderr);

	one = nemoshow_search_one(show, "main");
	if (one != NULL) {
		nemoshow_one_dump(one, stderr);
	}

	expr = nemoshow_expr_create();
	nemoshow_expr_add_symbol_table(expr, show->stable);

	nemoshow_expr_add_symbol(show->stable, "x", 3.0f);
	NEMO_DEBUG("expr = %f\n", nemoshow_expr_dispatch_expression(expr, "3 * pi * x"));

	nemoshow_expr_add_symbol(show->stable, "x", 6.0f);
	NEMO_DEBUG("expr = %f\n", nemoshow_expr_dispatch_expression(expr, "3 * pi * x"));

	nemoshow_expr_destroy(expr);

	nemoshow_destroy(show);

	return 0;
}
