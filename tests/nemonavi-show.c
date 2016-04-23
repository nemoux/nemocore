#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemonavi.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static void nemonavi_dispatch_paint(struct nemonavi *navi, const void *buffer, int width, int height, int dx, int dy, int dw, int dh)
{
}

int main(int argc, char *argv[])
{
	struct nemonavi *navi;

	nemonavi_init_once(argc, argv);

	navi = nemonavi_create("http://www.google.com");
	nemonavi_set_size(navi, 640, 480);
	nemonavi_set_dispatch_paint(navi, nemonavi_dispatch_paint);

	while (1)
		nemonavi_loop_once();

	nemonavi_destroy(navi);

	nemonavi_exit_once();

	return 0;
}
