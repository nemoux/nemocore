#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <playback.h>
#include <nemotool.h>
#include <nemoegl.h>
#include <talehelper.h>
#include <gsthelper.h>
#include <glibhelper.h>
#include <nemomisc.h>

static void playback_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);

	nemocanvas_feedback(canvas);

	nemotale_composite_egl(tale, NULL);
}

static void playback_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);
}

static void playback_dispatch_video_frame(GstElement *base, GstVideoFormat format, guint8 *buffer, gsize size, gpointer data)
{
	struct playback *play = (struct playback *)data;

	nemotale_node_attach_pixman(play->node, (void *)buffer, play->width, play->height);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "file",				required_argument,			NULL,		'f' },
		{ "width",			required_argument,			NULL,		'w' },
		{ "height",			required_argument,			NULL,		'h' },
		{ "background",	no_argument,						NULL,		'b' },
		{ 0 }
	};
	GMainLoop *gmainloop;
	struct playback *play;
	struct nemotool *tool;
	struct eglcontext *egl;
	struct eglcanvas *canvas;
	struct nemotale *tale;
	struct talenode *node;
	const char *filepath = NULL;
	char *uri;
	int32_t width = 1920;
	int32_t height = 1080;
	int opt;
	int i;

	while (opt = getopt_long(argc, argv, "f:w:h:b", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'f':
				filepath = strdup(optarg);
				break;

			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	if (filepath == NULL)
		return 0;

	gst_init(&argc, &argv);

	play = (struct playback *)malloc(sizeof(struct playback));
	if (play == NULL)
		return -1;
	memset(play, 0, sizeof(struct playback));

	play->width = width;
	play->height = height;

	play->tool = tool = nemotool_create();
	if (tool == NULL)
		return -1;
	nemotool_connect_wayland(tool, NULL);

	play->egl = egl = nemotool_create_egl(tool);

	play->eglcanvas = canvas = nemotool_create_egl_canvas(egl, width, height);
	nemocanvas_opaque(NTEGL_CANVAS(canvas), 0, 0, width, height);
	nemocanvas_set_nemosurface(NTEGL_CANVAS(canvas), NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_layer(NTEGL_CANVAS(canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	nemocanvas_set_dispatch_frame(NTEGL_CANVAS(canvas), playback_dispatch_canvas_frame);

	play->canvas = NTEGL_CANVAS(canvas);

	play->tale = tale = nemotale_create_gl();
	nemotale_set_backend(tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(egl),
				NTEGL_CONTEXT(egl),
				NTEGL_CONFIG(egl),
				(EGLNativeWindowType)NTEGL_WINDOW(canvas)));
	nemotale_resize(tale, width, height);

	nemotale_attach_canvas(tale, NTEGL_CANVAS(canvas), playback_dispatch_tale_event);
	nemotale_set_userdata(tale, play);

	play->node = node = nemotale_node_create_pixman(width, height);
	nemotale_node_set_id(node, 1);
	nemotale_node_opaque(node, 0, 0, width, height);
	nemotale_attach_node(tale, node);

	play->gst = nemogst_create();
	if (play->gst == NULL)
		goto out;

	asprintf(&uri, "file://%s", filepath);

	nemogst_prepare_mini_sink(play->gst,
			playback_dispatch_video_frame,
			play);
	nemogst_set_video_path(play->gst, uri);

	free(uri);

	nemogst_resize_video(play->gst, width, height);

	nemogst_play_video(play->gst);

	nemocanvas_dispatch_frame(play->canvas);

	gmainloop = g_main_loop_new(NULL, FALSE);

	nemoglib_run_tool(gmainloop, play->tool);

	g_main_loop_unref(gmainloop);

	nemogst_destroy(play->gst);

out:
	nemotale_destroy_gl(tale);

	nemotool_destroy_egl_canvas(canvas);
	nemotool_destroy_egl(egl);

	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

	free(play);

	return 0;
}
