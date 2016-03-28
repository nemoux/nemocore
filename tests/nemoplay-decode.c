#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoplay.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ 0 }
	};
	struct nemoplay *play;
	char *filepath = NULL;
	int opt;

	while (opt = getopt_long(argc, argv, "", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			default:
				break;
		}
	}

	if (optind < argc)
		filepath = strdup(argv[optind]);

	if (filepath == NULL)
		return 0;

	play = nemoplay_create();
	if (play == NULL)
		goto out1;

	if (nemoplay_open(play, filepath) < 0)
		goto out2;

out2:
	nemoplay_destroy(play);

out1:
	free(filepath);

	return 0;
}
