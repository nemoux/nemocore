#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <timer.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoitem.h>
#include <udphelper.h>
#include <nemomisc.h>

static void nemoenvs_dispatch_beat_timer(struct nemotimer *timer, void *data)
{
	struct nemoenvs *envs = (struct nemoenvs *)data;
	const char *msg = "live";

	udp_send_to(envs->beat.soc, "127.0.0.1", envs->beat.port, msg, strlen(msg) + 1);

	nemotimer_set_timeout(envs->beat.timer, envs->beat.timeout);
}

int nemoenvs_check_beats(struct nemoenvs *envs, int port, int timeout)
{
	char *msg;

	envs->beat.port = port;
	envs->beat.timeout = timeout;

	envs->beat.soc = udp_create_socket(NULL, 0);
	if (envs->beat.soc < 0)
		return -1;

	asprintf(&msg, "set %d \"%s\"", getpid(), envs->args);
	udp_send_to(envs->beat.soc, "127.0.0.1", envs->beat.port, msg, strlen(msg) + 1);
	free(msg);

	envs->beat.timer = nemotimer_create(envs->shell->compz);
	nemotimer_set_callback(envs->beat.timer, nemoenvs_dispatch_beat_timer);
	nemotimer_set_timeout(envs->beat.timer, envs->beat.timeout);
	nemotimer_set_userdata(envs->beat.timer, envs);

	return 0;
}
