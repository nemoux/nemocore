#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <getopt.h>

#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "socketpath",		required_argument,	NULL,		's' },
		{ 0 }
	};
	struct sockaddr_un addr;
	socklen_t size, namesize;
	char *socketpath = NULL;
	char msg[1024];
	int opt;
	int lsoc;
	int csoc;
	int len;

	while (opt = getopt_long(argc, argv, "s:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 's':
				socketpath = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (socketpath == NULL)
		socketpath = strdup("/tmp/nemo.log.0");

	unlink(socketpath);

	lsoc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (lsoc < 0)
		goto out0;

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), socketpath);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (bind(lsoc, (struct sockaddr *)&addr, size) < 0)
		goto out1;

	if (listen(lsoc, 1) < 0)
		goto out1;

	while ((csoc = accept(lsoc, (struct sockaddr *)&addr, &size)) >= 0) {
		while ((len = read(csoc, msg, sizeof(msg))) > 0) {
			msg[len] = '\0';

			printf(msg);
		}

		close(csoc);
	}

out1:
	close(lsoc);

out0:
	free(socketpath);

	return 0;
}
