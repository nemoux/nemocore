#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemotool.h>
#include <edgeback.h>
#include <edgeroll.h>
#include <edgemisc.h>
#include <nemograb.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static int nemoback_edge_dispatch_roll_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct nemograb *grab = (struct nemograb *)container_of(base, struct nemograb, base);
	struct edgeroll *roll = (struct edgeroll *)nemograb_get_userdata(grab);
	struct edgeback *edge = roll->edge;

	if (type & NEMOTALE_DOWN_EVENT) {
		nemoback_edgeroll_down(edge, roll, event->x, event->y);

		nemoshow_dispatch_frame(edge->show);
	} else if (type & NEMOTALE_MOTION_EVENT) {
		nemoback_edgeroll_motion(edge, roll, event->x, event->y);

		nemoshow_dispatch_frame(edge->show);
	} else if (type & NEMOTALE_UP_EVENT) {
		nemoback_edgeroll_up(edge, roll, event->x, event->y);

		nemoshow_dispatch_frame(edge->show);

		nemograb_destroy(grab);

		return 0;
	}

	return 1;
}

static int nemoback_edge_dispatch_roll_group_grab(struct talegrab *base, uint32_t type, struct taleevent *event)
{
	struct nemograb *grab = (struct nemograb *)container_of(base, struct nemograb, base);
	struct edgeroll *roll = (struct edgeroll *)nemograb_get_userdata(grab);
	struct edgeback *edge = roll->edge;
	struct nemoshow *show = edge->show;

	if (type & NEMOTALE_DOWN_EVENT) {
		nemoback_edgeroll_activate_group(edge, roll, nemograb_get_tag(grab));

		roll->tapcount++;
	} else if (type & NEMOTALE_MOTION_EVENT) {
		uint32_t taggroup;
		uint32_t tag, group;

		taggroup = nemoshow_canvas_pick_tag(edge->canvas, event->x, event->y);

		tag = NEMOSHOW_ONE_TAG(taggroup);
		group = NEMOSHOW_ONE_GROUP(taggroup);

		if (roll->serial == group && 1000 <= tag && tag < 2000) {
			if (roll->groupidx != tag - 1000) {
				nemoback_edgeroll_deactivate_group(edge, roll);
				nemoback_edgeroll_activate_group(edge, roll, tag - 1000);
			} else {
				nemoback_edgeroll_deactivate_action(edge, roll);
			}
		} else if (roll->serial == group && 2000 <= tag && tag < 3000) {
			if (roll->actionidx < 0) {
				nemoback_edgeroll_activate_action(edge, roll, tag - 2000);
			} else if (roll->actionidx != tag - 2000) {
				nemoback_edgeroll_deactivate_action(edge, roll);
				nemoback_edgeroll_activate_action(edge, roll, tag - 2000);
			}
		} else {
			nemoback_edgeroll_deactivate_action(edge, roll);
		}
	} else if (type & NEMOTALE_UP_EVENT) {
		roll->tapcount--;

		if (roll->actionidx >= 0) {
			nemocanvas_execute_action(NEMOSHOW_AT(show, canvas),
					roll->groupidx, roll->actionidx,
					NEMO_SURFACE_EXECUTE_TYPE_NORMAL,
					NEMO_SURFACE_COORDINATE_TYPE_GLOBAL,
					event->x, event->y, roll->r);

			nemoback_edgeroll_shutdown(edge, roll);
		} else {
			nemoback_edgeroll_deactivate_group(edge, roll);
		}

		nemograb_destroy(grab);

		return 0;
	}

	return 1;
}

static void nemoback_edge_dispatch_tale_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		if (nemotale_dispatch_grab(tale, event->device, type, event) == 0) {
			struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
			struct edgeback *edge = (struct edgeback *)nemoshow_get_userdata(show);
			struct showone *canvas = edge->canvas;
			struct nemograb *grab;

			if (nemotale_is_touch_down(tale, event, type)) {
				struct showone *one;
				uint32_t tag;

				one = nemoshow_canvas_pick_one(canvas, event->x, event->y);

				tag = NEMOSHOW_ONE_TAG(nemoshow_one_get_tag(one));
				if (1000 <= tag && tag < 2000) {
					struct edgeroll *roll = (struct edgeroll *)nemoshow_one_get_userdata(one);

					if (roll->tapcount == 0) {
						grab = nemograb_create(tale, event, nemoback_edge_dispatch_roll_group_grab);
						nemograb_set_userdata(grab, roll);
						nemograb_set_tag(grab, tag - 1000);
						nemograb_check_signal(grab, &roll->destroy_signal);
						nemotale_dispatch_grab(tale, event->device, type, event);
					}
				} else {
					int site = nemoback_edge_get_edge(edge, event->x, event->y, edge->rollrange);

					if (site == EDGEBACK_NONE_SITE) {
						nemotool_bypass_touch(edge->tool, event->device, event->x, event->y);
					} else {
						struct edgeroll *roll;

						roll = nemoback_edgeroll_create(edge, site);

						grab = nemograb_create(tale, event, nemoback_edge_dispatch_roll_grab);
						nemograb_set_userdata(grab, roll);
						nemograb_check_signal(grab, &roll->destroy_signal);
						nemotale_dispatch_grab(tale, event->device, type, event);
					}
				}
			}
		}
	}
}

static void nemoback_edge_dispatch_canvas_fullscreen(struct nemocanvas *canvas, int32_t active, int32_t opaque)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct edgeback *edge = (struct edgeback *)nemoshow_get_userdata(show);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,			NULL,		'w' },
		{ "height",				required_argument,			NULL,		'h' },
		{ "rollsize",			required_argument,			NULL,		's' },
		{ "rollrange",		required_argument,			NULL,		'r' },
		{ "rolltimeout",	required_argument,			NULL,		't' },
		{ "layer",				required_argument,			NULL,		'y' },
		{ "config",				required_argument,			NULL,		'c' },
		{ "log",					required_argument,			NULL,		'l' },
		{ 0 }
	};

	struct edgeback *edge;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *blur;
	struct showone *ease;
	struct showone *one;
	int32_t width = 1920;
	int32_t height = 1080;
	float rollsize = 100.0f;
	float rollrange = 50.0f;
	uint32_t rolltimeout = 1500;
	char *layer = NULL;
	char *configpath = NULL;
	int opt;
	int i;

	nemolog_set_file(2);

	while (opt = getopt_long(argc, argv, "w:h:s:r:t:y:c:l:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'w':
				width = strtoul(optarg, NULL, 10);
				break;

			case 'h':
				height = strtoul(optarg, NULL, 10);
				break;

			case 's':
				rollsize = strtod(optarg, NULL);
				break;

			case 'r':
				rollrange = strtod(optarg, NULL);
				break;

			case 't':
				rolltimeout = strtoul(optarg, NULL, 10);
				break;

			case 'y':
				layer = strdup(optarg);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			case 'l':
				nemolog_open_socket(optarg);
				break;

			default:
				break;
		}
	}

	if (configpath == NULL)
		asprintf(&configpath, "%s/.config/minishell.xml", getenv("HOME"));

	edge = (struct edgeback *)malloc(sizeof(struct edgeback));
	if (edge == NULL)
		return -1;
	memset(edge, 0, sizeof(struct edgeback));

	edge->width = width;
	edge->height = height;

	edge->rollsize = rollsize;
	edge->rollrange = rollrange;
	edge->rolltimeout = rolltimeout;

	edge->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err1;
	nemotool_connect_wayland(tool, NULL);

	edge->show = show = nemoshow_create_canvas(tool, width, height, nemoback_edge_dispatch_tale_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, edge);

	if (layer == NULL || strcmp(layer, "underlay") == 0) {
		nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_UNDERLAY);
	} else if (strcmp(layer, "overlay") == 0) {
		nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_OVERLAY);
	} else {
		nemocanvas_opaque(NEMOSHOW_AT(show, canvas), 0, 0, width, height);
		nemocanvas_set_layer(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_LAYER_TYPE_BACKGROUND);
	}

	nemocanvas_set_input_type(NEMOSHOW_AT(show, canvas), NEMO_SURFACE_INPUT_TYPE_TOUCH);
	nemocanvas_set_dispatch_fullscreen(NEMOSHOW_AT(show, canvas), nemoback_edge_dispatch_canvas_fullscreen);
	nemocanvas_unset_sound(NEMOSHOW_AT(show, canvas));

	edge->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_attach_one(show, scene);

	edge->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	edge->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, width, height);

	edge->ease0 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_INOUT_TYPE);

	edge->ease1 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_CUBIC_OUT_TYPE);

	edge->ease2 = ease = nemoshow_ease_create();
	nemoshow_ease_set_type(ease, NEMOEASE_LINEAR_TYPE);

	edge->inner = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 3.0f);

	edge->outer = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 3.0f);

	edge->solid = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "solid", 5.0f);

	edge->envs = nemoenvs_create();
	nemoenvs_load_configs(edge->envs, configpath);
	nemoenvs_load_actions(edge->envs);

	nemoshow_dispatch_frame(edge->show);

	nemotool_run(tool);

	nemoenvs_destroy(edge->envs);

err3:
	nemoshow_destroy_canvas(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(edge);

	return 0;
}
