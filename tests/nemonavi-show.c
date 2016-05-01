#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <getopt.h>

#include <nemonavi.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define NEMONAVI_MESSAGE_TIMEOUT		(8)

struct navicontext {
	struct nemotool *tool;

	struct nemoshow *show;
	struct showone *scene;
	struct showone *back;

	struct showone *view;
	struct showone *popup;

	struct nemonavi *navi;

	struct nemotimer *timer;
};

static void nemonavi_dispatch_paint(struct nemonavi *navi, int type, const void *buffer, int width, int height, int dx, int dy, int dw, int dh)
{
	struct navicontext *context = (struct navicontext *)nemonavi_get_userdata(navi);

	if (type == NEMONAVI_PAINT_VIEW_TYPE) {
		GLuint texture = nemoshow_canvas_get_texture(context->view);

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

		nemoshow_canvas_damage_all(context->view);
		nemoshow_dispatch_frame(context->show);
	} else if (type == NEMONAVI_PAINT_POPUP_TYPE) {
		GLuint texture = nemoshow_canvas_get_texture(context->popup);

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

		nemoshow_canvas_damage_all(context->popup);
		nemoshow_dispatch_frame(context->show);
	}
}

static void nemonavi_dispatch_popup_show(struct nemonavi *navi, int show)
{
	struct navicontext *context = (struct navicontext *)nemonavi_get_userdata(navi);

	if (show != 0)
		nemoshow_one_attach(context->scene, context->popup);
	else
		nemoshow_one_detach(context->popup);
}

static void nemonavi_dispatch_popup_rect(struct nemonavi *navi, int x, int y, int width, int height)
{
	struct navicontext *context = (struct navicontext *)nemonavi_get_userdata(navi);
	float x0, y0;
	float x1, y1;

	nemoshow_event_transform_from_viewport(context->show, x, y, &x0, &y0);
	nemoshow_event_transform_from_viewport(context->show, x + width, y + height, &x1, &y1);

	nemoshow_canvas_translate(context->popup, x0, y0);
	nemoshow_canvas_set_size(context->show, context->popup, x1 - x0, y1 - y0);
}

static void nemonavi_dispatch_key_event(struct nemonavi *navi, uint32_t code, int focus_on_editable_field)
{
	struct navicontext *context = (struct navicontext *)nemonavi_get_userdata(navi);

	if (focus_on_editable_field == 0) {
		if (code == 8)
			nemonavi_go_back(context->navi);
		else if (code == 171)
			nemonavi_set_zoomlevel(context->navi,
					nemonavi_get_zoomlevel(context->navi) + 0.5f);
		else if (code == 173)
			nemonavi_set_zoomlevel(context->navi,
					nemonavi_get_zoomlevel(context->navi) - 0.5f);
	}
}

static void nemonavi_dispatch_loading_state(struct nemonavi *navi, int is_loading, int can_go_back, int can_go_forward)
{
}

static void nemonavi_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct navicontext *context = (struct navicontext *)nemoshow_get_userdata(show);
	float x, y;

	if (nemoshow_event_is_touch_down(show, event) || nemoshow_event_is_touch_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_more_taps(show, event, 3)) {
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
		if (nemoshow_event_has_special_key(event) == 0)
			nemonavi_send_keyboard_down_event(context->navi,
					nemotool_get_keysym(context->tool, nemoshow_event_get_value(event)));
	} else if (nemoshow_event_is_keyboard_up(show, event)) {
		if (nemoshow_event_has_special_key(event) == 0)
			nemonavi_send_keyboard_up_event(context->navi,
					nemotool_get_keysym(context->tool, nemoshow_event_get_value(event)));
	}

	nemoshow_event_update_taps(show, canvas, event);

	if (nemoshow_event_is_more_taps(show, event, 2) == 0) {
		if (nemoshow_event_is_touch_down(show, event)) {
			int id = nemonavi_get_touchid_empty(context->navi);

			nemoshow_set_keyboard_focus(show, canvas);

			nemoshow_event_transform_to_viewport(show,
					nemoshow_event_get_x(event),
					nemoshow_event_get_y(event),
					&x, &y);

			nemoshow_event_set_tag(event, id + 1);

			nemonavi_send_touch_down_event(context->navi, x, y, id, nemoshow_event_get_time(event) / 1000.0f);
		} else if (nemoshow_event_is_touch_up(show, event)) {
			int id = nemoshow_event_get_tag(event);

			if (id > 0) {
				nemoshow_event_transform_to_viewport(show,
						nemoshow_event_get_x(event),
						nemoshow_event_get_y(event),
						&x, &y);

				nemonavi_send_touch_up_event(context->navi, x, y, id - 1, nemoshow_event_get_time(event) / 1000.0f);
			}
		} else if (nemoshow_event_is_touch_motion(show, event)) {
			int id = nemoshow_event_get_tag(event);

			if (id > 0) {
				nemoshow_event_transform_to_viewport(show,
						nemoshow_event_get_x(event),
						nemoshow_event_get_y(event),
						&x, &y);

				nemonavi_put_touchid(context->navi, id);

				nemonavi_send_touch_motion_event(context->navi, x, y, id - 1, nemoshow_event_get_time(event) / 1000.0f);
			}
		}
	}

	nemonavi_do_message();
}

static void nemonavi_dispatch_canvas_resize(struct nemoshow *show, struct showone *one, int32_t width, int32_t height)
{
	struct navicontext *context = (struct navicontext *)nemoshow_get_userdata(show);

	nemonavi_set_size(context->navi, width, height);

	nemonavi_do_message();
}

static void nemonavi_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct navicontext *context = (struct navicontext *)data;

	nemonavi_do_message();

	nemotimer_set_timeout(timer, NEMONAVI_MESSAGE_TIMEOUT);
}

int main(int argc, char *argv[])
{
	struct option options[] = {
		{ "url",				required_argument,			NULL,			'u' },
		{ "file",				required_argument,			NULL,			'f' },
		{ 0 }
	};

	struct navicontext *context;
	struct nemotool *tool;
	struct nemotimer *timer;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct nemonavi *navi;
	char *url = NULL;
	char *file = NULL;
	int width = 640;
	int height = 480;
	int opt;

	opterr = 0;

	while (opt = getopt_long(argc, argv, "u:f:", options, NULL)) {
		if (opt == -1)
			break;

		switch (opt) {
			case 'u':
				url = strdup(optarg);
				break;

			case 'f':
				file = strdup(optarg);
				break;

			default:
				break;
		}
	}

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

	context->view = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemonavi_dispatch_canvas_event);
	nemoshow_canvas_set_dispatch_resize(canvas, nemonavi_dispatch_canvas_resize);
	nemoshow_one_attach(scene, canvas);

	context->popup = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemonavi_dispatch_canvas_event);

	context->navi = navi = nemonavi_create("http://www.google.com");
	nemonavi_set_size(navi, width, height);
	nemonavi_set_dispatch_paint(navi, nemonavi_dispatch_paint);
	nemonavi_set_dispatch_popup_show(navi, nemonavi_dispatch_popup_show);
	nemonavi_set_dispatch_popup_rect(navi, nemonavi_dispatch_popup_rect);
	nemonavi_set_dispatch_key_event(navi, nemonavi_dispatch_key_event);
	nemonavi_set_dispatch_loading_state(navi, nemonavi_dispatch_loading_state);
	nemonavi_set_userdata(navi, context);

	if (file != NULL)
		nemonavi_load_page(navi, file);
	else if (url != NULL)
		nemonavi_load_url(navi, url);

	context->timer = timer = nemotimer_create(tool);
	nemotimer_set_callback(timer, nemonavi_dispatch_timer);
	nemotimer_set_userdata(timer, context);
	nemotimer_set_timeout(timer, NEMONAVI_MESSAGE_TIMEOUT);

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
