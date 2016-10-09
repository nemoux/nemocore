#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>
#include <ctype.h>

#include <nemoshow.h>
#include <showhelper.h>
#include <fbohelper.h>
#include <glhelper.h>
#include <glfilter.h>
#include <glripple.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct effectcontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct glfilter *filter;
	struct glripple *ripple;

	float width, height;

	int step;
};

static void nemoeffect_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct effectcontext *context = (struct effectcontext *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_touch_down(show, event) && context->ripple != NULL) {
		glripple_shoot(context->ripple,
				nemoshow_event_get_x(event) / context->width,
				nemoshow_event_get_y(event) / context->height,
				context->step);
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_more_taps(show, event, 3)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemoeffect_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct effectcontext *context = (struct effectcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	if (context->filter != NULL)
		glfilter_resize(context->filter, width, height);
	if (context->ripple != NULL)
		glripple_resize(context->ripple, width, height);

	nemoshow_view_redraw(context->show);
}

static GLuint nemoeffect_dispatch_tale_effect(struct talenode *node, void *data)
{
	struct effectcontext *context = (struct effectcontext *)data;
	GLuint texture = nemotale_node_get_texture(node);

	if (context->filter != NULL) {
		glfilter_dispatch(context->filter, texture);

		texture = glfilter_get_texture(context->filter);
	}

	if (context->ripple != NULL) {
		glripple_update(context->ripple);
		glripple_dispatch(context->ripple, texture);

		texture = glripple_get_texture(context->ripple);
	}

	return texture;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "program",				required_argument,			NULL,			'p' },
		{ "image",					required_argument,			NULL,			'i' },
		{ "step",						required_argument,			NULL,			's' },
		{ 0 }
	};

	struct effectcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	struct talenode *node;
	char *programpath = NULL;
	char *imagepath = NULL;
	int width = 800;
	int height = 800;
	int step = 7;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "p:i:s:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'p':
				programpath = strdup(optarg);
				break;

			case 'i':
				imagepath = strdup(optarg);
				break;

			case 's':
				step = strtoul(optarg, NULL, 10);
				break;

			default:
				break;
		}
	}

	context = (struct effectcontext *)malloc(sizeof(struct effectcontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct effectcontext));

	context->width = width;
	context->height = height;

	context->step = step;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemoeffect_dispatch_show_resize);
	nemoshow_set_userdata(show, context);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 1.0f, 1.0f, 1.0f, 1.0f);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoeffect_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	if (imagepath != NULL) {
		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_uri(one, imagepath);
	}

	node = nemoshow_canvas_get_node(canvas);
	nemotale_node_set_dispatch_effect(node, nemoeffect_dispatch_tale_effect, context);

	if (programpath != NULL)
		context->filter = glfilter_create(width, height, programpath);
	context->ripple = glripple_create(width, height);

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, context->canvas, NEMOSHOW_FILTER_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	if (context->filter != NULL)
		glfilter_destroy(context->filter);
	if (context->ripple != NULL)
		glripple_destroy(context->ripple);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
