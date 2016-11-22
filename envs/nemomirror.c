#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <shell.h>
#include <compz.h>
#include <view.h>
#include <canvas.h>
#include <seat.h>
#include <pointer.h>
#include <keyboard.h>
#include <touch.h>

#include <nemomirror.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemomisc.h>

static void nemomirror_dispatch_show_event(struct nemoshow *show, struct showevent *event)
{
	struct nemomirror *mirror = (struct nemomirror *)nemoshow_get_userdata(show);
	struct nemocompz *compz = mirror->shell->compz;

	if (nemoshow_event_is_pointer_enter(show, event)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_enter(pointer, mirror->view->content);
		}
	} else if (nemoshow_event_is_pointer_leave(show, event)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_leave(pointer, mirror->view->content);
		}
	}
}

static void nemomirror_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, struct showevent *event)
{
	struct nemomirror *mirror = (struct nemomirror *)nemoshow_get_userdata(show);
	struct nemocompz *compz = mirror->shell->compz;

	if (nemoshow_event_is_touch_down(show, event)) {
		struct touchpoint *tp;

		tp = nemoseat_get_touchpoint_by_id_nocheck(compz->seat, nemoshow_event_get_device(event));
		if (tp != NULL) {
			nemocontent_touch_down(tp, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_device(event),
					nemoshow_event_get_x(event) * mirror->view->content->width / nemoshow_canvas_get_width(mirror->canvas),
					nemoshow_event_get_y(event) * mirror->view->content->height / nemoshow_canvas_get_height(mirror->canvas),
					nemoshow_event_get_gx(event),
					nemoshow_event_get_gy(event));
		}
	} else if (nemoshow_event_is_touch_up(show, event)) {
		struct touchpoint *tp;

		tp = nemoseat_get_touchpoint_by_id_nocheck(compz->seat, nemoshow_event_get_device(event));
		if (tp != NULL) {
			nemocontent_touch_up(tp, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_device(event));
		}
	} else if (nemoshow_event_is_touch_motion(show, event)) {
		struct touchpoint *tp;

		tp = nemoseat_get_touchpoint_by_id_nocheck(compz->seat, nemoshow_event_get_device(event));
		if (tp != NULL) {
			nemocontent_touch_motion(tp, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_device(event),
					nemoshow_event_get_x(event) * mirror->view->content->width / nemoshow_canvas_get_width(mirror->canvas),
					nemoshow_event_get_y(event) * mirror->view->content->height / nemoshow_canvas_get_height(mirror->canvas),
					nemoshow_event_get_gx(event),
					nemoshow_event_get_gy(event));
		}
	}

	if (nemoshow_event_is_pointer_button_down(show, event, 0)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_button(pointer, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_value(event),
					WL_POINTER_BUTTON_STATE_PRESSED);
		}
	} else if (nemoshow_event_is_pointer_button_up(show, event, 0)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_button(pointer, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_value(event),
					WL_POINTER_BUTTON_STATE_RELEASED);
		}
	} else if (nemoshow_event_is_pointer_motion(show, event)) {
		struct nemopointer *pointer;

		pointer = nemoseat_get_pointer_by_id(compz->seat, nemoshow_event_get_device(event));
		if (pointer != NULL) {
			nemocontent_pointer_motion(pointer, mirror->view->content,
					nemoshow_event_get_time(event),
					nemoshow_event_get_x(event) * mirror->view->content->width / nemoshow_canvas_get_width(mirror->canvas),
					nemoshow_event_get_y(event) * mirror->view->content->height / nemoshow_canvas_get_height(mirror->canvas));
		}
	}
}

static GLuint nemomirror_dispatch_canvas_filter(struct talenode *node, void *data)
{
	struct nemomirror *mirror = (struct nemomirror *)data;
	GLuint texture = nemotale_node_get_texture(node);

	if (mirror->view != NULL) {
		texture = nemocanvas_get_opengl_texture(mirror->view->canvas, 0);
	}

	return texture;
}

static void nemomirror_handle_view_damage(struct wl_listener *listener, void *data)
{
	struct nemomirror *mirror = (struct nemomirror *)container_of(listener, struct nemomirror, view_damage_listener);

	nemoshow_canvas_damage_filter(mirror->canvas);

	nemoshow_view_redraw(mirror->show);
}

static void nemomirror_handle_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemomirror *mirror = (struct nemomirror *)container_of(listener, struct nemomirror, view_destroy_listener);

	nemomirror_destroy(mirror);
}

struct nemomirror *nemomirror_create(struct nemoshell *shell, int32_t x, int32_t y, int32_t width, int32_t height, const char *layer)
{
	struct nemomirror *mirror;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct talenode *node;

	mirror = (struct nemomirror *)malloc(sizeof(struct nemomirror));
	if (mirror == NULL)
		return NULL;
	memset(mirror, 0, sizeof(struct nemomirror));

	mirror->shell = shell;

	wl_list_init(&mirror->view_damage_listener.link);
	wl_list_init(&mirror->view_destroy_listener.link);

	wl_list_init(&mirror->destroy_listener.link);

	mirror->show = show = nemoshow_create_view(shell, width, height);
	nemoshow_set_dispatch_event(show, nemomirror_dispatch_show_event);
	nemoshow_view_set_position(show, x, y);
	nemoshow_view_set_layer(show, layer);

	mirror->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width);
	nemoshow_scene_set_height(scene, height);
	nemoshow_set_scene(show, scene);

	mirror->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 255.0f);
	nemoshow_canvas_set_alpha(canvas, 1.0f);
	nemoshow_one_attach(scene, canvas);

	mirror->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width);
	nemoshow_canvas_set_height(canvas, height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_OPENGL_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemomirror_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	node = nemoshow_canvas_get_node(canvas);
	nemotale_node_set_dispatch_filter(node, nemomirror_dispatch_canvas_filter, mirror);

	nemoshow_set_keyboard_focus(show, canvas);
	nemoshow_set_userdata(show, mirror);
	nemoshow_dispatch_frame(show);

	return mirror;
}

void nemomirror_destroy(struct nemomirror *mirror)
{
	wl_list_remove(&mirror->view_damage_listener.link);
	wl_list_remove(&mirror->view_destroy_listener.link);

	wl_list_remove(&mirror->destroy_listener.link);

	nemoshow_revoke_view(mirror->show);
	nemoshow_destroy_view_on_idle(mirror->show);

	free(mirror);
}

int nemomirror_set_view(struct nemomirror *mirror, struct nemoview *view)
{
	if (mirror->view == view)
		return 0;

	if (mirror->view != NULL) {
		wl_list_remove(&mirror->view_damage_listener.link);
		wl_list_init(&mirror->view_damage_listener.link);
		wl_list_remove(&mirror->view_destroy_listener.link);
		wl_list_init(&mirror->view_destroy_listener.link);
	}

	mirror->view = view;

	if (view != NULL) {
		mirror->view_damage_listener.notify = nemomirror_handle_view_damage;
		wl_signal_add(&view->canvas->damage_signal, &mirror->view_damage_listener);
		mirror->view_destroy_listener.notify = nemomirror_handle_view_destroy;
		wl_signal_add(&view->canvas->destroy_signal, &mirror->view_destroy_listener);
	}

	nemoshow_dispatch_frame(mirror->show);

	return 0;
}

static void nemomirror_handle_screen_destroy(struct wl_listener *listener, void *data)
{
	struct nemomirror *mirror = (struct nemomirror *)container_of(listener, struct nemomirror, destroy_listener);

	nemomirror_destroy(mirror);
}

int nemomirror_check_screen(struct nemomirror *mirror, struct shellscreen *screen)
{
	mirror->destroy_listener.notify = nemomirror_handle_screen_destroy;
	wl_signal_add(&screen->kill_signal, &mirror->destroy_listener);

	return 0;
}
