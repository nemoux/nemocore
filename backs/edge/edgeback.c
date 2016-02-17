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
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static int nemoback_edge_dispatch_roll_grab(struct nemoshow *show, void *data, uint32_t tag, void *event)
{
	struct edgeroll *roll = (struct edgeroll *)data;
	struct edgeback *edge = roll->edge;

	if (nemoshow_event_is_down(show, event)) {
		nemoback_edgeroll_down(edge, roll, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		nemoshow_dispatch_frame(show);
	} else if (nemoshow_event_is_motion(show, event)) {
		nemoback_edgeroll_motion(edge, roll, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		nemoshow_dispatch_frame(show);
	} else if (nemoshow_event_is_up(show, event)) {
		nemoback_edgeroll_up(edge, roll, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		nemoshow_dispatch_frame(show);

		return 0;
	}

	return 1;
}

static int nemoback_edge_dispatch_roll_group_grab(struct nemoshow *show, void *data, uint32_t tag, void *event)
{
	struct edgeroll *roll = (struct edgeroll *)data;
	struct edgeback *edge = roll->edge;

	if (nemoshow_event_is_down(show, event)) {
		nemoback_edgeroll_activate_group(edge, roll, tag);

		roll->tapcount++;
	} else if (nemoshow_event_is_motion(show, event)) {
		uint32_t taggroup;
		uint32_t tag, group;

		taggroup = nemoshow_canvas_pick_tag(edge->canvas, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

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
	} else if (nemoshow_event_is_up(show, event)) {
		roll->tapcount--;

		if (roll->actionidx >= 0) {
			nemocanvas_execute_action(NEMOSHOW_AT(show, canvas),
					roll->groupidx, roll->actionidx,
					NEMO_SURFACE_EXECUTE_TYPE_NORMAL,
					NEMO_SURFACE_COORDINATE_TYPE_GLOBAL,
					nemoshow_event_get_x(event),
					nemoshow_event_get_y(event),
					roll->r);

			nemoback_edgeroll_shutdown(edge, roll);
		} else {
			nemoback_edgeroll_deactivate_group(edge, roll);
		}

		return 0;
	}

	return 1;
}

static void nemoback_edge_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct edgeback *edge = (struct edgeback *)nemoshow_get_userdata(show);

	if (nemoshow_event_is_down(show, event)) {
		struct showgrab *grab;
		struct showone *one;
		uint32_t tag;

		one = nemoshow_canvas_pick_one(canvas, nemoshow_event_get_x(event), nemoshow_event_get_y(event));

		tag = NEMOSHOW_ONE_TAG(nemoshow_one_get_tag(one));
		if (1000 <= tag && tag < 2000) {
			struct edgeroll *roll = (struct edgeroll *)nemoshow_one_get_userdata(one);

			if (roll->tapcount == 0) {
				grab = nemoshow_grab_create(show, event, nemoback_edge_dispatch_roll_group_grab);
				nemoshow_grab_set_userdata(grab, roll);
				nemoshow_grab_set_tag(grab, tag - 1000);
				nemoshow_grab_check_signal(grab, &roll->destroy_signal);
				nemoshow_dispatch_grab(show, event);
			}
		} else {
			int site = nemoback_edge_get_edge(edge, nemoshow_event_get_x(event), nemoshow_event_get_y(event), edge->rollrange);

			if (site == EDGEBACK_NONE_SITE) {
				nemotool_bypass_touch(edge->tool,
						nemoshow_event_get_device(event),
						nemoshow_event_get_x(event),
						nemoshow_event_get_y(event));
			} else {
				struct edgeroll *roll;

				roll = nemoback_edgeroll_create(edge, site);

				grab = nemoshow_grab_create(show, event, nemoback_edge_dispatch_roll_grab);
				nemoshow_grab_set_userdata(grab, roll);
				nemoshow_grab_check_signal(grab, &roll->destroy_signal);
				nemoshow_dispatch_grab(show, event);
			}
		}
	}
}

static void nemoback_edge_dispatch_canvas_fullscreen(struct nemoshow *show, int32_t active, int32_t opaque)
{
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "width",				required_argument,			NULL,		'w' },
		{ "height",				required_argument,			NULL,		'h' },
		{ "rollsize",			required_argument,			NULL,		's' },
		{ "rollrange",		required_argument,			NULL,		'r' },
		{ "rolltimeout",	required_argument,			NULL,		't' },
		{ "layer",				required_argument,			NULL,		'l' },
		{ "config",				required_argument,			NULL,		'c' },
		{ "log",					required_argument,			NULL,		'o' },
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

	while (opt = getopt_long(argc, argv, "w:h:s:r:t:l:c:o:", options, NULL)) {
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

			case 'l':
				layer = strdup(optarg);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			case 'o':
				nemolog_open_socket(optarg);
				break;

			default:
				break;
		}
	}

	if (configpath == NULL)
		asprintf(&configpath, "%s/.config/minishell.xml", getenv("HOME"));
	if (layer == NULL)
		layer = strdup("underlay");

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

	edge->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err2;
	nemoshow_set_dispatch_fullscreen(show, nemoback_edge_dispatch_canvas_fullscreen);
	nemoshow_set_userdata(show, edge);

	nemoshow_view_set_layer(show, layer);
	nemoshow_view_set_input(show, "touch");
	nemoshow_view_put_sound(show);

	if (strcmp(layer, "background") == 0)
		nemoshow_view_set_opaque(show, 0, 0, width, height);

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
	nemoshow_canvas_set_dispatch_event(canvas, nemoback_edge_dispatch_canvas_event);
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
	nemoshow_destroy_view(show);

err2:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err1:
	free(edge);

	return 0;
}
