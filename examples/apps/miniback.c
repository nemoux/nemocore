#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <miniback.h>

#include <nemotool.h>
#include <nemoegl.h>
#include <talehelper.h>
#include <nemomisc.h>

static void miniback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

static void miniback_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "type",				required_argument,			NULL,		't' },
		{ 0 }
	};
	struct miniback *mini;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	const char *type = NULL;
	int opt;

	while (opt = getopt_long(argc, argv, "t:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 't':
				type = strdup(optarg);
				break;

			default:
				break;
		}
	}

	mini = (struct miniback *)malloc(sizeof(struct miniback));
	if (mini == NULL)
		return -1;
	memset(mini, 0, sizeof(struct miniback));

	mini->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	mini->egl = egl = nemotool_create_egl(tool);

	mini->eglcanvas = canvas = nemotool_create_egl_canvas(egl, 1920, 1080);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(NTEGL_CANVAS(canvas), miniback_dispatch_canvas_resize);

	mini->canvas = NTEGL_CANVAS(canvas);

	mini->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, 1920, 1080);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), miniback_dispatch_tale_event);
	nemotale_set_userdata(tale, mini);

	node = nemotale_node_create_pixman(1920, 1080);
	nemotale_node_set_id(node, 1);
	nemotale_attach_node(tale, node);
	nemotale_node_fill_pixman(node, 1.0f, 0.0f, 0.0f, 1.0f);

	nemotale_composite_egl(tale, NULL);

	nemotool_run(tool);

	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(mini);

	return 0;
}
