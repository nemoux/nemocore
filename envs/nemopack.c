#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>

#include <pixman.h>

#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <shell.h>
#include <compz.h>
#include <view.h>
#include <canvas.h>
#include <seat.h>
#include <touch.h>
#include <move.h>
#include <pick.h>
#include <timer.h>
#include <nemoshell.h>
#include <viewanimation.h>

#include <nemopack.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

void __attribute__((constructor(101))) nemopack_initialize_envs(void)
{
}

void __attribute__((destructor(101))) nemopack_finalize_envs(void)
{
}

static void nemopack_handle_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemopack *pack = (struct nemopack *)container_of(listener, struct nemopack, view_destroy_listener);

	nemopack_destroy(pack);
}

static void nemopack_handle_bin_resize(struct wl_listener *listener, void *data)
{
	struct nemopack *pack = (struct nemopack *)container_of(listener, struct nemopack, bin_resize_listener);
	uint32_t width = pack->view->content->width;
	uint32_t height = pack->view->content->height;

	nemoshow_dispatch_resize(pack->show, width, height);
	nemoshow_dispatch_frame(pack->show);
}

static void nemopack_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct nemopack *pack = (struct nemopack *)nemoshow_get_userdata(show);
	struct nemoshell *shell = pack->shell;
	struct nemocompz *compz = shell->compz;
	struct nemoseat *seat = compz->seat;

	if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
		nemoshow_event_update_taps(show, canvas, event);

		if (nemoshow_event_is_no_tap(show, event)) {
			nemotimer_set_timeout(pack->timer, pack->timeout);
		} else if (nemoshow_event_is_single_tap(show, event)) {
			struct touchpoint *tp;

			tp = nemoseat_get_touchpoint_by_id(seat, nemoshow_event_get_device_on(event, 0));
			if (tp != NULL) {
				nemoshell_move_canvas_by_touchpoint(shell, tp, pack->bin);

				nemotimer_set_timeout(pack->timer, 0);
			}
		} else if (nemoshow_event_is_double_taps(show, event)) {
			struct touchpoint *tp0, *tp1;

			tp0 = nemoseat_get_touchpoint_by_id(seat, nemoshow_event_get_device_on(event, 0));
			tp1 = nemoseat_get_touchpoint_by_id(seat, nemoshow_event_get_device_on(event, 1));
			if (tp0 != NULL && tp1 != NULL) {
				if (pack->bin->flags & NEMO_SHELL_SURFACE_RESIZABLE_FLAG) {
					nemoshell_pick_canvas_by_touchpoint_on_area(shell, tp0, tp1, pack->bin);
				} else if (pack->bin->flags & NEMO_SHELL_SURFACE_SCALABLE_FLAG) {
					nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALEONLY), pack->bin);
				} else {
					nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE), pack->bin);
				}

				nemotimer_set_timeout(pack->timer, 0);
			}
		} else if (nemoshow_event_is_many_taps(show, event)) {
			struct touchpoint *tp0, *tp1;
			uint64_t device0, device1;

			nemoshow_event_get_distant_tapdevices(show, event, &device0, &device1);

			tp0 = nemoseat_get_touchpoint_by_id(seat, device0);
			tp1 = nemoseat_get_touchpoint_by_id(seat, device1);
			if (tp0 != NULL && tp1 != NULL) {
				if (pack->bin->flags & NEMO_SHELL_SURFACE_SCALABLE_FLAG) {
					nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE) | (1 << NEMO_SURFACE_PICK_TYPE_SCALEONLY), pack->bin);
				} else {
					nemoshell_pick_canvas_by_touchpoint(shell, tp0, tp1, (1 << NEMO_SURFACE_PICK_TYPE_ROTATE) | (1 << NEMO_SURFACE_PICK_TYPE_MOVE), pack->bin);
				}

				nemotimer_set_timeout(pack->timer, 0);
			}
		}
	}
}

static void nemopack_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemopack *pack = (struct nemopack *)data;

	nemopack_destroy(pack);
}

struct nemopack *nemopack_create(struct nemoshell *shell, struct nemoview *view, uint32_t timeout)
{
	struct nemopack *pack;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *one;
	struct showone *blur;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *set0;
	struct showone *set1;

	pack = (struct nemopack *)malloc(sizeof(struct nemopack));
	if (pack == NULL)
		return NULL;
	memset(pack, 0, sizeof(struct nemopack));

	pack->shell = shell;

	pack->timeout = timeout;

	pack->timer = nemotimer_create(shell->compz);
	if (pack->timer == NULL)
		goto err1;
	nemotimer_set_callback(pack->timer, nemopack_dispatch_timer);
	nemotimer_set_userdata(pack->timer, pack);

	pack->show = show = nemoshow_create_view(shell, view->content->width, view->content->height);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, pack);

	pack->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, view->content->width);
	nemoshow_scene_set_height(scene, view->content->height);
	nemoshow_set_scene(show, scene);

	pack->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, view->content->width);
	nemoshow_canvas_set_height(canvas, view->content->height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_one_attach(scene, canvas);

	pack->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, view->content->width);
	nemoshow_canvas_set_height(canvas, view->content->height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemopack_dispatch_canvas_event);
	nemoshow_one_attach(scene, canvas);

	pack->blur0 = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(blur, "high", "inner", 5.0f);
	pack->blur1 = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_filter_set_blur(blur, "high", "outer", 0.0f);

	pack->one = one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, view->content->width);
	nemoshow_item_set_height(one, view->content->height);
	nemoshow_item_set_fill_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_item_set_filter(one, pack->blur0);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, one);
	nemoshow_sequence_set_cattr(set0, "fill", 0x1e, 0xdc, 0xdc, 0x40);

	pack->icon = one = nemoshow_item_create(NEMOSHOW_PATHTWICE_ITEM);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_width(one, view->content->width / 2);
	nemoshow_item_set_height(one, view->content->height / 2);
	nemoshow_item_translate(one, view->content->width / 4, view->content->height / 4);
	nemoshow_item_path_load_svg(one, NEMOENVS_RESOURCES "/misc-icons/catch.svg", 0.0f, 0.0f, view->content->width / 2, view->content->height / 2);
	nemoshow_item_set_fill_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_item_set_stroke_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_item_set_stroke_width(one, MIN(view->content->width, view->content->height) / 64);
	nemoshow_item_set_filter(one, pack->blur1);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, one);
	nemoshow_sequence_set_cattr(set1, "fill", 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_sequence_set_cattr(set1, "stroke", 0x1e, 0xdc, 0xdc, 0xff);

	sequence = nemoshow_sequence_create_easy(show,
			nemoshow_sequence_create_frame_easy(show,
				1.0f, set0, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_OUT_EASE, 500, 0);
	nemoshow_transition_check_one(trans, one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(show, trans);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, pack->blur1);
	nemoshow_sequence_set_dattr(set0, "r", 8.0f);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, pack->blur1);
	nemoshow_sequence_set_dattr(set1, "r", 3.0f);

	sequence = nemoshow_sequence_create_easy(show,
			nemoshow_sequence_create_frame_easy(show,
				0.5f, set0, NULL),
			nemoshow_sequence_create_frame_easy(show,
				1.0f, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(NEMOSHOW_CUBIC_INOUT_EASE, 1200, 0);
	nemoshow_transition_check_one(trans, pack->blur1);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	nemoshow_view_set_parent(show, view);

	pack->view = view;
	pack->bin = nemoshell_get_bin(view->canvas);

	pack->view_destroy_listener.notify = nemopack_handle_view_destroy;
	wl_signal_add(&view->destroy_signal, &pack->view_destroy_listener);

	pack->bin_resize_listener.notify = nemopack_handle_bin_resize;
	wl_signal_add(&pack->bin->resize_signal, &pack->bin_resize_listener);

	nemolist_init(&pack->destroy_listener.link);

	nemoshow_dispatch_frame(show);

	return pack;

err2:
	nemotimer_destroy(pack->timer);

err1:
	free(pack);

	return NULL;
}

void nemopack_destroy(struct nemopack *pack)
{
	nemolist_remove(&pack->destroy_listener.link);

	wl_list_remove(&pack->view_destroy_listener.link);
	wl_list_remove(&pack->bin_resize_listener.link);

	nemotimer_destroy(pack->timer);

	nemoshow_one_destroy(pack->blur0);
	nemoshow_one_destroy(pack->blur1);

	nemoshow_one_destroy(pack->one);
	nemoshow_one_destroy(pack->icon);
	nemoshow_one_destroy(pack->back);
	nemoshow_one_destroy(pack->scene);

	nemoshow_put_scene(pack->show);

	nemoshow_revoke_view(pack->show);
	nemoshow_destroy_view_on_idle(pack->show);

	free(pack);
}
