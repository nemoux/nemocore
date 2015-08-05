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
	struct showtransition *trans;

	show = nemoshow_create();
	nemoshow_load_xml(show, argv[1]);
	nemoshow_arrange_one(show);

	nemoshow_update_symbol(show, "hour", 5.0f);
	nemoshow_update_symbol(show, "min", 10.0f);
	nemoshow_update_symbol(show, "sec", 15.0f);
	nemoshow_update_expression(show);

	nemoshow_dump_all(show, stderr);

	trans = nemoshow_transition_create(
			nemoshow_search_one(show, "ease1"),
			3000, 1000,
			nemoshow_get_next_serial(show));
	nemoshow_transition_attach_sequence(trans,
			nemoshow_search_one(show, "hour-text-sequence"));

	while (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, time_current_msecs());
	}

	nemoshow_destroy(show);

	return 0;
}
