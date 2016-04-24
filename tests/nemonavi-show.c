#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemonavi.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

struct navicontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;
	struct showone *canvas;

	struct nemonavi *navi;

	struct nemotimer *timer;
};

static void nemonavi_dispatch_paint(struct nemonavi *navi, const void *buffer, int width, int height, int dx, int dy, int dw, int dh)
{
	struct navicontext *context = (struct navicontext *)nemonavi_get_userdata(navi);
	GLuint texture = nemoshow_canvas_get_texture(context->canvas);

	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, width);

	if (dx == 0 && dy == 0 && dw == width && dh == height) {
		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, 0);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, width, height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);
	} else {
		glPixelStorei(GL_UNPACK_SKIP_PIXELS_EXT, dx);
		glPixelStorei(GL_UNPACK_SKIP_ROWS_EXT, dy);

		glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, dw, dh, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	nemoshow_canvas_damage_all(context->canvas);
	nemoshow_dispatch_frame(context->show);
}

static void nemonavi_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct navicontext *context = (struct navicontext *)nemoshow_get_userdata(show);
	float x, y;

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_serial_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ALL_TYPE);
		}
	}

	if (nemoshow_event_is_pointer_enter(show, event)) {
		nemoshow_event_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_enter_event(context->navi, x, y);
	} else if (nemoshow_event_is_pointer_leave(show, event)) {
		nemoshow_event_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_leave_event(context->navi, x, y);
	} else if (nemoshow_event_is_pointer_button_down(show, event, 0)) {
		nemoshow_set_keyboard_focus(show, canvas);

		nemoshow_event_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_down_event(context->navi, x, y,
				nemoshow_event_get_value(event));
	} else if (nemoshow_event_is_pointer_button_up(show, event, 0)) {
		nemoshow_event_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_up_event(context->navi, x, y,
				nemoshow_event_get_value(event));
	} else if (nemoshow_event_is_pointer_motion(show, event)) {
		nemoshow_event_transform_to_viewport(show,
				nemoshow_event_get_x(event),
				nemoshow_event_get_y(event),
				&x, &y);

		nemonavi_send_pointer_motion_event(context->navi, x, y);
	} else if (nemoshow_event_is_pointer_axis(show, event)) {
		if (nemoshow_event_get_axis(event) == NEMO_POINTER_AXIS_ROTATE_X) {
			nemonavi_send_pointer_wheel_event(context->navi, 0.0f, nemoshow_event_get_r(event) < 0.0f ? 40.0f : -40.0f);
		} else {
			nemonavi_send_pointer_wheel_event(context->navi, nemoshow_event_get_r(event) < 0.0f ? 40.0f : -40.0f, 0.0f);
		}
	}

	if (nemoshow_event_is_keyboard_down(show, event)) {
		if (nemotool_is_special_key(nemoshow_event_get_value(event)) == 0)
			nemonavi_send_keyboard_down_event(context->navi,
					nemotool_get_keysym(context->tool, nemoshow_event_get_value(event)));
	} else if (nemoshow_event_is_keyboard_up(show, event)) {
		if (nemotool_is_special_key(nemoshow_event_get_value(event)) == 0)
			nemonavi_send_keyboard_up_event(context->navi,
					nemotool_get_keysym(context->tool, nemoshow_event_get_value(event)));
	}

	if (nemoshow_event_is_touch_down(show, event)) {
	} else if (nemoshow_event_is_touch_up(show, event)) {
	} else if (nemoshow_event_is_touch_motion(show, event)) {
	}
}

static void nemonavi_dispatch_canvas_resize(struct nemoshow *show, struct showone *one, int32_t width, int32_t height)
{
	struct navicontext *context = (struct navicontext *)nemoshow_get_userdata(show);

	nemonavi_set_size(context->navi, width, height);
}

static void nemonavi_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct navicontext *context = (struct navicontext *)data;

	nemonavi_loop_once();

	nemotimer_set_timeout(timer, 30);
}

int main(int argc, char *argv[])
{
	struct navicontext *context;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct nemonavi *navi;
	int width = 640;
	int height = 480;

	nemonavi_init_once(argc, argv);

	context = (struct navicontext *)malloc(sizeof(struct navicontext));
	if (context == NULL)
		goto err1;
	memset(context, 0, sizeof(struct navicontext));

	context->tool = tool = nemotool_create();
	if (tool == NULL)
		goto err2;
	nemotool_connect_wayland(tool, NULL);

	context->show = show = nemoshow_create_view(tool, width, height);
	if (show == NULL)
		goto err3;
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
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemonavi_dispatch_canvas_event);
	nemoshow_canvas_set_dispatch_resize(canvas, nemonavi_dispatch_canvas_resize);
	nemoshow_one_attach(scene, canvas);

	context->navi = navi = nemonavi_create("http://www.google.com");
	nemonavi_set_size(navi, width, height);
	nemonavi_set_dispatch_paint(navi, nemonavi_dispatch_paint);
	nemonavi_set_userdata(navi, context);

	context->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemonavi_dispatch_timer);
	nemotimer_set_userdata(timer, context);
	nemotimer_set_timeout(timer, 30);

	nemoshow_dispatch_frame(show);

	nemotool_run(tool);

	nemotimer_destroy(timer);

	nemonavi_destroy(navi);

	nemoshow_destroy_view(show);

err3:
	nemotool_disconnect_wayland(tool);
	nemotool_destroy(tool);

err2:
	free(context);

err1:
	nemonavi_exit_once();

	return 0;
}
