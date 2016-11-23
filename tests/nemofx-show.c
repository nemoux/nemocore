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
#include <glshader.h>
#include <glfilter.h>
#include <glblur.h>
#include <glripple.h>
#include <gllight.h>
#include <glshadow.h>
#include <glswirl.h>
#include <fxnoise.h>
#include <nemohelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct glfxcontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct glfilter *filter;
	struct glblur *blur;
	struct glripple *ripple;
	struct gllight *light;
	struct glshadow *shadow;
	struct glswirl *swirl;
	struct fxnoise *noise;

	float width, height;

	int ripplestep;

	float lightscope;
	float shadowscope;
};

static void nemoglfx_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct glfxcontext *context = (struct glfxcontext *)nemoshow_get_userdata(show);

	nemoshow_event_update_taps(show, canvas, event);

	if (context->ripple != NULL) {
		if (nemoshow_event_is_touch_down(show, event)) {
			nemofx_glripple_shoot(context->ripple,
					nemoshow_event_get_x(event) / context->width,
					nemoshow_event_get_y(event) / context->height,
					context->ripplestep);
		}
	}

	if (context->light != NULL) {
		if (nemoshow_event_is_touch_down(show, event) ||
				nemoshow_event_is_touch_motion(show, event) ||
				nemoshow_event_is_touch_up(show, event)) {
			int tapcount = nemoshow_event_get_tapcount(event);
			int i;

			nemofx_gllight_clear_pointlights(context->light);

			for (i = 0; i < tapcount; i++) {
				nemofx_gllight_set_pointlight_position(context->light, i,
						nemoshow_event_get_x_on(event, i) / context->width,
						nemoshow_event_get_y_on(event, i) / context->height);
				nemofx_gllight_set_pointlight_scope(context->light, i, context->lightscope);
				nemofx_gllight_set_pointlight_size(context->light, i, context->lightscope / 8.0f);
			}
		}
	}

	if (context->shadow != NULL) {
		if (nemoshow_event_is_touch_down(show, event) ||
				nemoshow_event_is_touch_motion(show, event) ||
				nemoshow_event_is_touch_up(show, event)) {
			int tapcount = nemoshow_event_get_tapcount(event);
			int i;

			nemofx_glshadow_clear_pointlights(context->shadow);

			for (i = 0; i < tapcount; i++) {
				nemofx_glshadow_set_pointlight_position(context->shadow, i,
						nemoshow_event_get_x_on(event, i) / context->width,
						nemoshow_event_get_y_on(event, i) / context->height);
				nemofx_glshadow_set_pointlight_size(context->shadow, i, context->shadowscope / 8.0f);
			}
		}
	}

	if (context->swirl != NULL) {
		if (nemoshow_event_is_touch_motion(show, event)) {
			nemofx_glswirl_set_angle(context->swirl,
					(nemoshow_event_get_x(event) / context->width - 0.5f) * M_PI * 2.0f);
		}
	}

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		if (nemoshow_event_is_more_taps(show, event, 5)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);

			nemoshow_event_set_cancel(event);

			nemoshow_dispatch_grab_all(show, event);
		}
	}
}

static void nemoglfx_dispatch_show_resize(struct nemoshow *show, int32_t width, int32_t height)
{
	struct glfxcontext *context = (struct glfxcontext *)nemoshow_get_userdata(show);

	nemoshow_view_resize(context->show, width, height);

	if (context->noise != NULL) {
		nemofx_noise_dispatch(context->noise, nemoshow_canvas_map(context->back), width, height);
		nemoshow_canvas_unmap(context->back);
	}

	if (context->filter != NULL)
		nemofx_glfilter_resize(context->filter, width, height);
	if (context->blur != NULL)
		nemofx_glblur_resize(context->blur, width, height);
	if (context->light != NULL)
		nemofx_gllight_resize(context->light, width, height);
	if (context->shadow != NULL)
		nemofx_glshadow_resize(context->shadow, width, height);
	if (context->ripple != NULL)
		nemofx_glripple_resize(context->ripple, width, height);
	if (context->swirl != NULL)
		nemofx_glswirl_resize(context->swirl, width, height);

	nemoshow_view_redraw(context->show);
}

static GLuint nemoglfx_dispatch_canvas_filter(struct talenode *node, void *data)
{
	struct glfxcontext *context = (struct glfxcontext *)data;
	GLuint texture = nemotale_node_get_texture(node);

	if (context->filter != NULL) {
		nemofx_glfilter_dispatch(context->filter, texture);

		texture = nemofx_glfilter_get_texture(context->filter);
	}

	if (context->blur != NULL) {
		nemofx_glblur_dispatch(context->blur, texture);

		texture = nemofx_glblur_get_texture(context->blur);
	}

	if (context->light != NULL) {
		nemofx_gllight_dispatch(context->light, texture);

		texture = nemofx_gllight_get_texture(context->light);
	}

	if (context->shadow != NULL) {
		nemofx_glshadow_dispatch(context->shadow, texture);

		texture = nemofx_glshadow_get_texture(context->shadow);
	}

	if (context->ripple != NULL) {
		nemofx_glripple_update(context->ripple);
		nemofx_glripple_dispatch(context->ripple, texture);

		texture = nemofx_glripple_get_texture(context->ripple);
	}

	if (context->swirl != NULL) {
		nemofx_glswirl_dispatch(context->swirl, texture);

		texture = nemofx_glswirl_get_texture(context->swirl);
	}

	return texture;
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "program",				required_argument,			NULL,			'p' },
		{ "image",					required_argument,			NULL,			'i' },
		{ "ripple",					required_argument,			NULL,			'r' },
		{ "blur",						required_argument,			NULL,			'b' },
		{ "light",					required_argument,			NULL,			'l' },
		{ "shadow",					required_argument,			NULL,			's' },
		{ "noise",					required_argument,			NULL,			'n' },
		{ "swirl",					required_argument,			NULL,			'w' },
		{ 0 }
	};

	struct glfxcontext *context;
	struct nemotool *tool;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showtransition *trans;
	struct talenode *node;
	char *programpath = NULL;
	char *imagepath = NULL;
	char *noisetype = NULL;
	float lightscope = 0.0f;
	float shadowscope = 0.0f;
	float swirlradius = 0.0f;
	int width = 800;
	int height = 800;
	int ripplestep = 0;
	int blur = 0;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "p:i:r:b:l:s:n:w:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'p':
				programpath = strdup(optarg);
				break;

			case 'i':
				imagepath = strdup(optarg);
				break;

			case 'r':
				ripplestep = strtoul(optarg, NULL, 10);
				break;

			case 'b':
				blur = strtoul(optarg, NULL, 10);
				break;

			case 'l':
				lightscope = strtod(optarg, NULL);
				break;

			case 's':
				shadowscope = strtod(optarg, NULL);
				break;

			case 'n':
				noisetype = strdup(optarg);
				break;

			case 'w':
				swirlradius = strtod(optarg, NULL);
				break;

			default:
				break;
		}
	}

	context = (struct glfxcontext *)malloc(sizeof(struct glfxcontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct glfxcontext));

	context->width = width;
	context->height = height;

	context->ripplestep = ripplestep;

	context->lightscope = lightscope;
	context->shadowscope = shadowscope * width;

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
	nemoshow_set_dispatch_resize(show, nemoglfx_dispatch_show_resize);
	nemoshow_set_userdata(show, context);

	context->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	context->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_PIXMAN_TYPE);
	nemoshow_canvas_set_opaque(canvas, 1);
	nemoshow_one_attach(scene, canvas);

	context->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemoglfx_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	if (imagepath != NULL) {
		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_uri(one, imagepath);
	} else {
		one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, 0.0f);
		nemoshow_item_set_y(one, 0.0f);
		nemoshow_item_set_width(one, width);
		nemoshow_item_set_height(one, height);
		nemoshow_item_set_fill_color(one, 0.0f, 0.0f, 0.0f, 0.0f);

		one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_x(one, width / 4.0f);
		nemoshow_item_set_y(one, height / 4.0f);
		nemoshow_item_set_width(one, width / 2.0f);
		nemoshow_item_set_height(one, height / 2.0f);
		nemoshow_item_set_stroke_width(one, 5.0f);
		nemoshow_item_set_stroke_color(one, 0.0f, 255.0f, 255.0f, 255.0f);

		one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_cx(one, width / 2.0f);
		nemoshow_item_set_cy(one, height / 2.0f);
		nemoshow_item_set_r(one, width / 6.0f);
		nemoshow_item_set_fill_color(one, 0.0f, 255.0f, 255.0f, 255.0f);

		one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
		nemoshow_one_attach(canvas, one);
		nemoshow_item_set_cx(one, width / 1.0f);
		nemoshow_item_set_cy(one, height / 2.0f);
		nemoshow_item_set_r(one, width / 4.0f);
		nemoshow_item_set_stroke_width(one, 5.0f);
		nemoshow_item_set_stroke_color(one, 0.0f, 255.0f, 255.0f, 255.0f);
	}

	node = nemoshow_canvas_get_node(canvas);
	nemotale_node_set_dispatch_filter(node, nemoglfx_dispatch_canvas_filter, context);

	if (programpath != NULL) {
		context->filter = nemofx_glfilter_create(width, height);
		nemofx_glfilter_set_program(context->filter, programpath);
	}

	if (noisetype != NULL) {
		context->noise = nemofx_noise_create();
		nemofx_noise_set_type(context->noise, noisetype);
		nemofx_noise_dispatch(context->noise, nemoshow_canvas_map(context->back), width, height);
		nemoshow_canvas_unmap(context->back);
	}

	if (blur > 0) {
		context->blur = nemofx_glblur_create(width, height);
		nemofx_glblur_set_radius(context->blur, blur, blur);
	}

	if (ripplestep > 0) {
		context->ripple = nemofx_glripple_create(width, height);
		nemofx_glripple_use_vectors(context->ripple, NULL, 32, 32, 400, 400);
		nemofx_glripple_use_amplitudes(context->ripple, NULL, 2048, 18, 0.125f);
		nemofx_glripple_layout(context->ripple, 32, 32, 2048);
	}

	if (lightscope > 0.0f) {
		context->light = nemofx_gllight_create(width, height);
		nemofx_gllight_set_ambientlight_color(context->light, 0.0f, 0.0f, 0.0f);
		nemofx_gllight_set_pointlight_color(context->light, 0, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_gllight_set_pointlight_color(context->light, 1, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_gllight_set_pointlight_color(context->light, 2, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_gllight_set_pointlight_color(context->light, 3, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_gllight_set_pointlight_color(context->light, 4, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
	}

	if (shadowscope > 0.0f) {
		context->shadow = nemofx_glshadow_create(width, height, context->shadowscope);
		nemofx_glshadow_set_pointlight_color(context->shadow, 0, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_glshadow_set_pointlight_color(context->shadow, 1, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_glshadow_set_pointlight_color(context->shadow, 2, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_glshadow_set_pointlight_color(context->shadow, 3, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
		nemofx_glshadow_set_pointlight_color(context->shadow, 4, random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f), random_get_double(0.0f, 1.0f));
	}

	if (swirlradius > 0.0f) {
		context->swirl = nemofx_glswirl_create(width, height);
		nemofx_glswirl_set_radius(context->swirl, swirlradius);
		nemofx_glswirl_set_center(context->swirl, 0.0f, 0.0f);
	}

	trans = nemoshow_transition_create(NEMOSHOW_LINEAR_EASE, 18000, 0);
	nemoshow_transition_dirty_one(trans, context->canvas, NEMOSHOW_FILTER_DIRTY);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	if (context->filter != NULL)
		nemofx_glfilter_destroy(context->filter);
	if (context->blur != NULL)
		nemofx_glblur_destroy(context->blur);
	if (context->ripple != NULL)
		nemofx_glripple_destroy(context->ripple);
	if (context->light != NULL)
		nemofx_gllight_destroy(context->light);
	if (context->shadow != NULL)
		nemofx_glshadow_destroy(context->shadow);
	if (context->swirl != NULL)
		nemofx_glswirl_destroy(context->swirl);
	if (context->noise != NULL)
		nemofx_noise_destroy(context->noise);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	return 0;
}
