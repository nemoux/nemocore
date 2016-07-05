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

int nemoenvs_dispatch_config_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	if (strcmp(cmd, "set") == 0) {
		struct itemone *tone;
		const char *id;

		id = nemoitem_one_get_attr(one, "id");
		if (id != NULL) {
			tone = nemoitem_search_attr(envs->configs, path, "id", id);
			if (tone != NULL)
				nemoitem_one_copy_attr(tone, one);
			else
				nemoitem_attach_one(envs->configs,
						nemoitem_one_clone(one));
		} else {
			nemoitem_attach_one(envs->configs,
					nemoitem_one_clone(one));
		}
	} else if (strcmp(cmd, "get") == 0) {
		struct itemone *tone;
		const char *id;

		id = nemoitem_one_get_attr(one, "id");
		if (id != NULL) {
			tone = nemoitem_search_attr(envs->configs, path, "id", id);
			if (tone != NULL) {
				char contents[1024] = { 0 };

				nemoitem_one_save(tone, contents, ' ', '\"');

				nemoenvs_reply(envs, "%s %s set %s", dst, src, contents);
			}
		}
	} else if (strcmp(cmd, "put") == 0) {
		struct itemone *tone;
		const char *id;

		id = nemoitem_one_get_attr(one, "id");
		if (id != NULL) {
			tone = nemoitem_search_attr(envs->configs, path, "id", id);
			if (tone != NULL)
				nemoitem_one_destroy(tone);
		}
	}

	return 0;
}

int nemoenvs_dispatch_client_message(struct nemoenvs *envs, const char *src, const char *dst, const char *cmd, const char *path, struct itemone *one, void *data)
{
	struct nemotool *tool = (struct nemotool *)data;

	if (strcmp(cmd, "req") == 0) {
		if (strcmp(path, "/check/live") == 0) {
			nemoenvs_reply(envs, "%s %s rep /check/live", envs->clientname, src);
		}
	}

	return 0;
}
