#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <signal.h>
#include <sys/epoll.h>

#include <udphelper.h>
#include <oshelper.h>
#include <nemotoken.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "port",			required_argument,		NULL,		'p' },
		{ "timeout",	required_argument,		NULL,		't' },
		{ 0 }
	};
	struct nemotoken *token;
	struct epoll_event ep[16];
	const char *cmd;
	char ip[64];
	char msg[1024];
	char args[1024] = { 0 };
	int port = 50000;
	int timeout = 5;
	int pid = 0;
	int efd;
	int tfd;
	int soc;
	int opt;
	int count;
	int i;

	while (opt = getopt_long(argc, argv, "p:t:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'p':
				port = strtoul(optarg, NULL, 10);
				break;

			case 't':
				timeout = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	efd = os_epoll_create_cloexec();
	if (efd < 0)
		return -1;

	tfd = os_timerfd_create_cloexec();
	if (tfd < 0)
		goto out1;

	soc = udp_create_socket("0.0.0.0", port);
	if (soc < 0)
		goto out2;

	os_epoll_add_fd(efd, tfd, EPOLLIN | EPOLLERR | EPOLLHUP, (void *)0x1);
	os_epoll_add_fd(efd, soc, EPOLLIN | EPOLLERR | EPOLLHUP, (void *)0x2);

	while (1) {
		count = epoll_wait(efd, ep, ARRAY_LENGTH(ep), -1);

		for (i = 0; i < count; i++) {
			if (ep[i].data.ptr == (void *)0x1) {
				NEMO_DEBUG("timeout...pid(%d) args(%s)\n", pid, args);

				if (pid > 0) {
					NEMO_DEBUG("kill...pid(%d)\n", pid);

					kill(pid, SIGKILL);
					sleep(1);

					token = nemotoken_create(args, strlen(args));
					if (token != NULL) {
						nemotoken_divide(token, ';');
						nemotoken_update(token);

						NEMO_DEBUG("restart...args(%s)\n", args);

						if (fork() == 0) {
							execv(nemotoken_get_token(token, 0), nemotoken_get_tokens(token));
							exit(EXIT_FAILURE);
						}

						nemotoken_destroy(token);
					}
				}

				os_timerfd_set_timeout(tfd, 0, 0);
			} else if (ep[i].data.ptr == (void *)0x2) {
				udp_recv_from(soc, ip, &port, msg, sizeof(msg));

				token = nemotoken_create(msg, strlen(msg));
				if (token != NULL) {
					nemotoken_fence(token, '\"');
					nemotoken_divide(token, ' ');
					nemotoken_update(token);

					cmd = nemotoken_get_token(token, 0);

					if (strcmp(cmd, "set") == 0) {
						pid = nemotoken_get_integer(token, 1, 0);

						strcpy(args, nemotoken_get_string(token, 2, ""));

						NEMO_DEBUG("set...pid(%d) args(%s)\n", pid, args);
					} else if (strcmp(cmd, "live") == 0) {
						os_timerfd_set_timeout(tfd, timeout, 0);

						NEMO_DEBUG("live...\n");
					}

					nemotoken_destroy(token);
				}
			}
		}
	}

	close(soc);

out2:
	close(tfd);

out1:
	close(efd);

	return 0;
}
