#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <udphelper.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "ip",			required_argument,		NULL,		'i' },
		{ "port",		required_argument,		NULL,		'p' },
		{ 0 }
	};
	char ip[64] = "127.0.0.1";
	char msg[1024] = "hello";
	int port = 70000;
	int soc;
	int len;
	int opt;

	while (opt = getopt_long(argc, argv, "i:p:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'i':
				strcpy(ip, optarg);
				break;

			case 'p':
				port = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	soc = udp_create_socket(NULL, 0);
	if (soc < 0)
		return -1;

	udp_send_to(soc, ip, port, msg, strlen(msg) + 1);

	while (1) {
		len = udp_recv_from(soc, ip, &port, msg, sizeof(msg));

		msg[len] = '\0';

		printf("%s", msg);
	}

	close(soc);

	return 0;
}
