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
	struct showone *one;

	show = nemoshow_create();
	nemoshow_load_xml(show, argv[1]);
	nemoshow_update_one(show);

	nemoshow_update_symbol(show, "x", 5.0f);
	nemoshow_update_expression(show, "sec-hand", "height");

	nemoshow_dump_all(show, stderr);

	nemoshow_destroy(show);

	return 0;
}
