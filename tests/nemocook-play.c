#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemoegl.h>
#include <nemomisc.h>

struct cookcontext {
	struct nemotool *tool;
	struct nemoegl *egl;
	struct nemocanvas *canvas;

	int width, height;
};

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",			required_argument,		NULL,		'w' },
		{ "height",			required_argument,		NULL,		'h' },
		{ "fullscreen",	required_argument,		NULL,		'f' },
		{ 0 }
	};

	struct cookcontext *context;
	struct nemotool *tool;
	struct nemoegl *egl;
	struct nemocanvas *canvas;
	char *fullscreenid = NULL;
	char *contentpath = NULL;
	int width = 1920;
	int height = 1080;
	int opt;

	while (opt = getopt_long(argc, argv, "w:h:f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 'f':
				fullscreenid = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (optind < argc)
		contentpath = strdup(argv[optind]);

	if (contentpath == NULL)
		return 0;

	context = (struct cookcontext *)malloc(sizeof(struct cookcontext));
	if (context == NULL)
		return -1;
	memset(context, 0, sizeof(struct cookcontext));

	context->width = width;
	context->height = height;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	context->egl = egl = nemoegl_create(tool);
	if (egl == NULL)
		goto err2;

	context->canvas = canvas = nemoegl_create_canvas(egl, width, height);
	if (canvas == NULL)
		goto err3;

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);

	nemotool_run(tool);

	nemoegl_destroy_canvas(canvas);

err3:
	nemoegl_destroy(egl);

err2:
	nemotool_destroy(tool);

err1:
	free(context);

	return 0;
}
