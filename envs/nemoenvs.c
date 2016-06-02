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
#include <view.h>
#include <monitor.h>
#include <message.h>
#include <waylandhelper.h>

#include <nemoenvs.h>
#include <nemoapps.h>
#include <nemolist.h>
#include <nemoitem.h>
#include <nemoease.h>
#include <nemoxml.h>
#include <nemotoken.h>
#include <udphelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct nemoenvs *nemoenvs_create(struct nemoshell *shell)
{
	struct nemoenvs *envs;

	envs = (struct nemoenvs *)malloc(sizeof(struct nemoenvs));
	if (envs == NULL)
		return NULL;
	memset(envs, 0, sizeof(struct nemoenvs));

	envs->shell = shell;

	envs->configs = nemoitem_create();
	if (envs->configs == NULL)
		goto err1;

	nemolist_init(&envs->app_list);

	return envs;

err1:
	free(envs);

	return NULL;
}

void nemoenvs_destroy(struct nemoenvs *envs)
{
	nemolist_remove(&envs->app_list);

	nemoitem_destroy(envs->configs);

	free(envs);
}

static int nemoenvs_dispatch_message(void *data)
{
	struct nemomsg *msg = (struct nemomsg *)data;
	struct nemotoken *content;
	int soc = nemomsg_get_socket(msg);
	char buffer[1024];
	char ip[64];
	int port;
	int size;

	size = udp_recv_from(soc, ip, &port, buffer, sizeof(buffer) - 1);
	if (size <= 0)
		return -1;

	content = nemotoken_create(buffer, size);
	nemotoken_divide(content, ':');
	nemotoken_update(content);

	nemomsg_dispatch(msg, content);
	nemomsg_clean(msg);

	return 0;
}

int nemoenvs_listen(struct nemoenvs *envs, const char *ip, int port)
{
	envs->msg = nemomsg_create(ip, port);
	if (envs->msg == NULL)
		return -1;

	envs->monitor = nemomonitor_create(envs->shell->compz,
			nemomsg_get_socket(envs->msg),
			nemoenvs_dispatch_message,
			envs->msg);

	return 0;
}

void nemoenvs_load_configs(struct nemoenvs *envs, const char *configpath)
{
	struct nemoxml *xml;
	struct xmlnode *node;
	struct itemone *one;
	int i;

	xml = nemoxml_create();
	nemoxml_load_file(xml, configpath);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		one = nemoitem_one_create();
		nemoitem_one_set_path(one, node->path);

		for (i = 0; i < node->nattrs; i++) {
			nemoitem_one_set_attr(one,
					node->attrs[i*2+0],
					node->attrs[i*2+1]);
		}

		nemoitem_attach_one(envs->configs, one);

		nemoshell_dispatch_message(envs->shell, "set", node->path, one);
	}

	nemoxml_destroy(xml);
}
