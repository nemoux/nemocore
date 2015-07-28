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
	struct showone *text;
	double t;

	show = nemoshow_create();
	nemoshow_load_xml(show, argv[1]);
	nemoshow_sort_one(show);
	nemoshow_arrange_one(show);

	nemoshow_update_symbol(show, "hour", 5.0f);
	nemoshow_update_symbol(show, "min", 10.0f);
	nemoshow_update_symbol(show, "sec", 15.0f);
	nemoshow_update_expression(show);

	nemoshow_dump_all(show, stderr);

	one = nemoshow_search_one(show, "hour-text-sequence");
	nemoshow_sequence_prepare(one);

	text = nemoshow_search_one(show, "hour-text");

	for (t = 0.0f; t <= 1.01f; t += 0.1f) {
		nemoshow_sequence_update(one, t);

		NEMO_DEBUG("=> %f %f\n", NEMOSHOW_TEXT(text)->x, NEMOSHOW_TEXT(text)->y);
	}

	nemoshow_destroy(show);

	return 0;
}
