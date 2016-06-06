#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoenvs.h>
#include <nemoitem.h>
#include <nemomisc.h>

int nemoenvs_dispatch_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	struct nemotool *tool = (struct nemotool *)data;

	if (strcmp(cmd, "get") == 0) {
		if (strcmp(path, "/check/live") == 0) {
			nemoenvs_send(envs, "%s#%s#set#/check/live", envs->clientname, src);
		}
	}

	return 0;
}
