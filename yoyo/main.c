#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemoyoyo.h>
#include <yoyoone.h>
#include <yoyosweep.h>
#include <yoyoactor.h>
#include <nemojson.h>
#include <nemofs.h>
#include <nemomisc.h>

static int nemoyoyo_dispatch_tap_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemoaction_get_userdata(action);

	if (event & NEMOACTION_TAP_DOWN_EVENT) {
		struct yoyoone *one;

		one = nemoyoyo_pick_one(yoyo,
				nemoaction_tap_get_tx(tap),
				nemoaction_tap_get_ty(tap));
		if (one == NULL) {
			struct json_object *jobj;

			jobj = nemojson_search_object(yoyo->config, 0, 1, "sweep");
			if (jobj != NULL) {
				struct yoyosweep *sweep;

				sweep = nemoyoyo_sweep_create(yoyo);
				nemoyoyo_sweep_set_minimum_range(sweep, nemojson_object_get_double(jobj, "minimum_range", 50.0f));
				nemoyoyo_sweep_set_maximum_range(sweep, nemojson_object_get_double(jobj, "maximum_range", 150.0f));
				nemoyoyo_sweep_set_minimum_angle(sweep, nemojson_object_get_double(jobj, "minimum_angle", 180.0f) * M_PI / 180.0f);
				nemoyoyo_sweep_set_maximum_angle(sweep, nemojson_object_get_double(jobj, "maximum_angle", 540.0f) * M_PI / 180.0f);
				nemoyoyo_sweep_set_minimum_interval(sweep, nemojson_object_get_integer(jobj, "minimum_interval", 4));
				nemoyoyo_sweep_set_maximum_interval(sweep, nemojson_object_get_integer(jobj, "maximum_interval", 8));
				nemoyoyo_sweep_set_minimum_duration(sweep, nemojson_object_get_integer(jobj, "minimum_duration", 800));
				nemoyoyo_sweep_set_maximum_duration(sweep, nemojson_object_get_integer(jobj, "maximum_duration", 1200));
				nemoyoyo_sweep_set_feedback_sx0(sweep, nemojson_object_get_double(jobj, "feedback_sx0", 0.15f));
				nemoyoyo_sweep_set_feedback_sx1(sweep, nemojson_object_get_double(jobj, "feedback_sx1", 1.0f));
				nemoyoyo_sweep_set_feedback_sy0(sweep, nemojson_object_get_double(jobj, "feedback_sy0", 0.15f));
				nemoyoyo_sweep_set_feedback_sy1(sweep, nemojson_object_get_double(jobj, "feedback_sy1", 1.0f));
				nemoyoyo_sweep_set_feedback_alpha0(sweep, nemojson_object_get_double(jobj, "feedback_alpha0", 0.75f));
				nemoyoyo_sweep_set_feedback_alpha1(sweep, nemojson_object_get_double(jobj, "feedback_alpha1", 0.0f));
				nemoyoyo_sweep_set_actor_distance(sweep, nemojson_object_get_double(jobj, "actor_distance", 180.0f));
				nemoyoyo_sweep_set_actor_duration(sweep, nemojson_object_get_integer(jobj, "actor_duration", 300));
				nemoyoyo_sweep_dispatch(sweep, tap);
			}
		} else {
			nemoaction_tap_set_userdata(tap, one);
			nemoaction_tap_set_focus(action, tap, one);
			nemoaction_tap_dispatch_event(action, tap, NEMOACTION_TAP_DOWN_EVENT);
		}
	}

	return 0;
}

static void nemoyoyo_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemocanvas_get_userdata(canvas);

	if (nemotransition_group_dispatch(yoyo->transitions, time_current_msecs()) != 0)
		nemoyoyo_set_flags(yoyo, NEMOYOYO_REDRAW_FLAG);

	if (nemoyoyo_update_one(yoyo) != 0)
		nemoyoyo_set_flags(yoyo, NEMOYOYO_REDRAW_FLAG);

	if (nemoyoyo_has_flags(yoyo, NEMOYOYO_REDRAW_FLAG) != 0) {
		nemocanvas_dispatch_feedback(canvas);

		nemoyoyo_update_frame(yoyo);

		nemoyoyo_put_flags(yoyo, NEMOYOYO_REDRAW_FLAG);
	} else {
		nemocanvas_terminate_feedback(canvas);
	}
}

static int nemoyoyo_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemoyoyo *yoyo = (struct nemoyoyo *)nemocanvas_get_userdata(canvas);
	struct actiontap *tap;

	if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		tap = nemoaction_tap_create(yoyo->action);
		nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
		nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
		nemoaction_tap_set_time(tap, nemoevent_get_time(event));
		nemoaction_tap_set_grab_tx(tap, nemoevent_get_canvas_x(event));
		nemoaction_tap_set_grab_ty(tap, nemoevent_get_canvas_y(event));
		nemoaction_tap_set_grab_time(tap, nemoevent_get_time(event));
		nemoaction_tap_set_device(tap, nemoevent_get_device(event));
		nemoaction_tap_set_serial(tap, nemoevent_get_serial(event));
		nemoaction_tap_clear(tap,
				nemoevent_get_canvas_x(event),
				nemoevent_get_canvas_y(event));
		nemoaction_tap_dispatch_event(yoyo->action, tap, NEMOACTION_TAP_DOWN_EVENT);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		tap = nemoaction_get_tap_by_device(yoyo->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_time(tap, nemoevent_get_time(event));
			nemoaction_tap_detach(tap);
			nemoaction_tap_dispatch_event(yoyo->action, tap, NEMOACTION_TAP_UP_EVENT);
			nemoaction_tap_destroy(tap);
		}
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		tap = nemoaction_get_tap_by_device(yoyo->action, nemoevent_get_device(event));
		if (tap != NULL) {
			nemoaction_tap_set_tx(tap, nemoevent_get_canvas_x(event));
			nemoaction_tap_set_ty(tap, nemoevent_get_canvas_y(event));
			nemoaction_tap_trace(tap,
					nemoevent_get_canvas_x(event),
					nemoevent_get_canvas_y(event));
			nemoaction_tap_dispatch_event(yoyo->action, tap, NEMOACTION_TAP_MOTION_EVENT);
		}
	}

	return 0;
}

static int nemoyoyo_dispatch_canvas_destroy(struct nemocanvas *canvas)
{
	nemotool_exit(nemocanvas_get_tool(canvas));

	return 1;
}

static void nemoyoyo_dispatch_bus(void *data, const char *events)
{
}

int main(int argc, char *argv[])
{
	static const char *vertexshader_texture =
		"uniform mat4 transform;\n"
		"attribute vec2 position;\n"
		"attribute vec2 texcoord;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_Position = transform * vec4(position, 0.0, 1.0);\n"
		"  vtexcoord = texcoord;\n"
		"}\n";
	static const char *fragmentshader_texture =
		"precision mediump float;\n"
		"uniform sampler2D texture;\n"
		"uniform float alpha;\n"
		"varying vec2 vtexcoord;\n"
		"void main()\n"
		"{\n"
		"  gl_FragColor = texture2D(texture, vtexcoord);\n"
		"  gl_FragColor.a = gl_FragColor.a * alpha;\n"
		"}\n";

	struct option options[] = {
		{ "width",						required_argument,		NULL,		'w' },
		{ "height",						required_argument,		NULL,		'h' },
		{ "fullscreen",				required_argument,		NULL,		'f' },
		{ "layer",						required_argument,		NULL,		'y' },
		{ "config",						required_argument,		NULL,		'c' },
		{ "busid",						required_argument,		NULL,		'b' },
		{ 0 }
	};

	struct nemoyoyo *yoyo;
	struct nemotool *tool;
	struct nemocanvas *canvas;
	struct cookegl *egl;
	struct cookshader *shader;
	struct cooktrans *trans;
	struct nemoaction *action;
	struct nemojson *config;
	struct nemobus *bus;
	char *configpath = NULL;
	char *fullscreenid = NULL;
	char *layer = NULL;
	char *busid = NULL;
	int width = 1920;
	int height = 1080;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "w:h:f:y:c:b:", options, NULL)) {
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

			case 'y':
				layer = strdup(optarg);
				break;

			case 'c':
				configpath = strdup(optarg);
				break;

			case 'b':
				busid = strdup(optarg);
				break;

			default:
				break;
		}
	}

	if (configpath == NULL)
		return 0;

	yoyo = nemoyoyo_create();
	yoyo->busid = busid != NULL ? strdup(busid) : "/nemoyoyo";

	tool = yoyo->tool = nemotool_create();
	nemotool_connect_wayland(tool, NULL);
	nemotool_connect_egl(tool, 1, 4);

	canvas = yoyo->canvas = nemocanvas_egl_create(tool, width, height);
	nemocanvas_set_nemosurface(canvas, "normal");
	nemocanvas_set_dispatch_frame(canvas, nemoyoyo_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(canvas, nemoyoyo_dispatch_canvas_event);
	nemocanvas_set_dispatch_destroy(canvas, nemoyoyo_dispatch_canvas_destroy);
	nemocanvas_set_userdata(canvas, yoyo);

	if (fullscreenid != NULL)
		nemocanvas_set_fullscreen(canvas, fullscreenid);
	if (layer != NULL)
		nemocanvas_set_layer(canvas, layer);

	egl = yoyo->egl = nemocook_egl_create(
			NTEGL_DISPLAY(tool),
			NTEGL_CONTEXT(tool),
			NTEGL_CONFIG(tool),
			NTEGL_WINDOW(canvas));
	nemocook_egl_resize(egl, width, height);

	nemocook_egl_attach_state(egl,
			nemocook_state_create(1, NEMOCOOK_STATE_COLOR_BUFFER_TYPE, 0.0f, 0.0f, 0.0f, 0.0f));
	nemocook_egl_attach_state(egl,
			nemocook_state_create(2, NEMOCOOK_STATE_BLEND_SEPARATE_TYPE, 1, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

	shader = yoyo->shader = nemocook_shader_create();
	nemocook_shader_set_program(shader, vertexshader_texture, fragmentshader_texture);
	nemocook_shader_set_attrib(shader, 0, "position", 2);
	nemocook_shader_set_attrib(shader, 1, "texcoord", 2);
	nemocook_shader_set_uniform(shader, 0, "transform");
	nemocook_shader_set_uniform(shader, 1, "alpha");

	trans = yoyo->projection = nemocook_transform_create();
	nemocook_transform_set_translate(trans, -1.0f, 1.0f, 0.0f);
	nemocook_transform_set_scale(trans, 2.0f / (float)width, -2.0f / (float)height, 1.0f);
	nemocook_transform_set_state(trans, NEMOCOOK_TRANSFORM_NOPIN_STATE);
	nemocook_transform_update(trans);

	action = yoyo->action = nemoaction_create();
	nemoaction_set_tap_callback(action, nemoyoyo_dispatch_tap_event);
	nemoaction_set_userdata(action, yoyo);

	config = yoyo->config = nemojson_create();
	nemojson_append_file(config, configpath);
	nemojson_update(config);
	nemoyoyo_load_config(yoyo);

	bus = yoyo->bus = nemobus_create();
	nemobus_connect(bus, NULL);
	nemobus_advertise(bus, "set", yoyo->busid);
	nemotool_watch_source(tool,
			nemobus_get_socket(bus),
			"reh",
			nemoyoyo_dispatch_bus,
			yoyo);

	nemoyoyo_dispatch_frame(yoyo);

	nemotool_run(tool);

	nemobus_destroy(bus);
	nemojson_destroy(config);

	nemoaction_destroy(action);

	nemocook_transform_destroy(trans);
	nemocook_shader_destroy(shader);
	nemocook_egl_destroy(egl);

	nemocanvas_egl_destroy(canvas);

	nemotool_destroy(tool);

	nemoyoyo_destroy(yoyo);

	return 0;
}
