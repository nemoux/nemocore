#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/signalfd.h>

#include <nemolog.h>
#include <nemomisc.h>

int main(int argc, char *argv[])
{
	struct signalfd_siginfo info;
	sigset_t mask;
	ssize_t res;
	int pid;
	int fd;

	nemolog_set_file(2);

	nemolog_message("EXEC", "start '%s' file...\n", argv[1]);

	sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
		nemolog_error("EXEC", "failed to block child signal...\n");
		return -1;
	}

	fd = signalfd(-1, &mask, 0);
	if (fd < 0) {
		nemolog_error("EXEC", "failed to create signalfd...\n");
		return -1;
	}

	nemolog_message("EXEC", "execute '%s' file...\n", argv[1]);

	pid = fork();
	if (pid == 0) {
		execv(argv[1], &argv[1]);
		exit(0);
	}

	while (1) {
		res = read(fd, &info, sizeof(info));
		if (res < 0 || res != sizeof(info))
			break;

		if (info.ssi_signo == SIGCHLD) {
			nemolog_message("EXEC", "reexecute '%s' file...\n", argv[1]);

			pid = fork();
			if (pid == 0) {
				execv(argv[1], &argv[1]);
				exit(0);
			}
		} else {
			break;
		}
	}

	close(fd);

	nemolog_message("EXEC", "end '%s' file...\n", argv[1]);

	return 0;
}
