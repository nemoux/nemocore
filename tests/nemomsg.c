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
		{ "msg",		required_argument,		NULL,		'm' },
		{ "reply",	required_argument,		NULL,		'r' },
		{ 0 }
	};
	char *livemsg = "/nemomsg#/nemoshell#set#/check/live";
	char ip[64];
	char msg[1024];
	int needs_reply = 0;
	int port;
	int soc;
	int opt;

	while (opt = getopt_long(argc, argv, "i:p:m:r", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'i':
				strcpy(ip, optarg);
				break;

			case 'p':
				port = strtoul(optarg, NULL, 10);
				break;

			case 'm':
				strcpy(msg, optarg);
				break;

			case 'r':
				needs_reply = 1;
				break;

			default:
				break;
		}
	}

	soc = udp_create_socket(NULL, 0);
	if (soc < 0)
		return -1;

	udp_send_to(soc, ip, port, livemsg, strlen(livemsg) + 1);
	udp_send_to(soc, ip, port, msg, strlen(msg) + 1);

	while (needs_reply != 0) {
		char contents[1024];
		char ip[128];
		int port;

		udp_recv_from(soc, ip, &port, contents, sizeof(contents));

		NEMO_DEBUG("[%s:%d] %s\n", ip, port, contents);
	}

	close(soc);

	return 0;
}
