#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <waylandhelper.h>

#include <nemolist.h>
#include <nemoitem.h>
#include <nemomisc.h>

void nemoenvs_load_background(struct nemoshell *shell)
{
	struct nemocompz *compz = shell->compz;
	int index;

	nemoitem_for_each(shell->configs, index, "//nemoshell/background", 0) {
		pid_t pid;
		char *argv[32];
		char *attr;
		int argc = 0;
		int iattr;
		int32_t x, y;
		int32_t width, height;

		x = nemoitem_get_iattr(shell->configs, index, "x", 0);
		y = nemoitem_get_iattr(shell->configs, index, "y", 0);
		width = nemoitem_get_iattr(shell->configs, index, "width", nemocompz_get_scene_width(compz));
		height = nemoitem_get_iattr(shell->configs, index, "height", nemocompz_get_scene_height(compz));

		argv[argc++] = nemoitem_get_attr(shell->configs, index, "path");
		argv[argc++] = strdup("-w");
		asprintf(&argv[argc++], "%d", width);
		argv[argc++] = strdup("-h");
		asprintf(&argv[argc++], "%d", height);

		nemoitem_for_vattr(shell->configs, index, "arg%d", iattr, 0, attr)
			argv[argc++] = attr;

		argv[argc++] = NULL;

		pid = wayland_execute_path(argv[0], argv, NULL);
		if (pid > 0) {
			struct clientstate *state;

			state = nemoshell_create_client_state(shell, pid);
			if (state != NULL) {
				clientstate_set_position(state, x, y);
				clientstate_set_anchor(state, 0.0f, 0.0f);
				clientstate_set_bin_flags(state, NEMO_SHELL_SURFACE_ALL_FLAGS);
			}
		}

		free(argv[1]);
		free(argv[2]);
		free(argv[3]);
		free(argv[4]);
	}
}

void nemoenvs_load_soundmanager(struct nemoshell *shell)
{
	int index;

	index = nemoitem_get(shell->configs, "//nemoshell/sound", 0);
	if (index >= 0) {
		char *path;
		char *attr;
		char *argv[32];
		int argc = 0;
		int i;

		argv[argc++] = nemoitem_get_attr(shell->configs, index, "path");

		nemoitem_for_vattr(shell->configs, index, "arg%d", i, 0, attr)
			argv[argc++] = attr;

		argv[argc++] = NULL;

		wayland_execute_path(argv[0], argv, NULL);
	}
}
