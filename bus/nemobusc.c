#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemobus.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "socketpath",				required_argument,		NULL,		's' },
		{ "path",							required_argument,		NULL,		'p' },
		{ 0 }
	};
	struct nemobus *bus;
	struct busmsg *msg;
	char *socketpath = NULL;
	char *contents = NULL;
	char *path = NULL;
	int opt;

	while (opt = getopt_long(argc, argv, "s:p:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				socketpath = strdup(optarg);
				break;

			case 'p':
				path = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (optind < argc)
		contents = strdup(argv[optind]);

	if (contents == NULL)
		return -1;

	bus = nemobus_create();
	nemobus_connect(bus, socketpath);
	nemobus_advertise(bus, "/nemobusc");

	msg = nemobus_msg_from_json_string(contents);

	nemobus_send(bus, path, msg);

	nemobus_msg_destroy(msg);

	nemobus_destroy(bus);

	return 0;
}
