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
	struct showtransition *trans;
	int done;

	show = nemoshow_create();
	nemoshow_load_xml(show, argv[1]);
	nemoshow_sort_one(show);
	nemoshow_arrange_one(show);

	nemoshow_update_symbol(show, "hour", 5.0f);
	nemoshow_update_symbol(show, "min", 10.0f);
	nemoshow_update_symbol(show, "sec", 15.0f);
	nemoshow_update_expression(show);

	nemoshow_dump_all(show, stderr);

	one = nemoshow_search_one(show, "hour-text");

	trans = nemoshow_transition_create(
			nemoshow_search_one(show, "ease1"),
			3000,
			1000);

	nemoshow_transition_attach_sequence(trans,
			nemoshow_search_one(show, "hour-text-sequence"));

	do {
		done = nemoshow_transition_dispatch(trans, time_current_msecs());
	} while (done == 0);

	nemoshow_transition_destroy(trans);

	nemoshow_destroy(show);

	return 0;
}
