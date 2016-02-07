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
#include <layer.h>
#include <view.h>
#include <canvas.h>
#include <seat.h>
#include <touch.h>
#include <grab.h>
#include <move.h>
#include <pick.h>
#include <actor.h>
#include <timer.h>
#include <nemoshell.h>
#include <viewanimation.h>

#include <nemopack.h>
#include <talehelper.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

static struct showone *nemopackease0;
static struct showone *nemopackease1;

void __attribute__((constructor(101))) nemopack_prepare_envs(void)
{
	nemopackease0 = nemoshow_ease_create();
	nemoshow_ease_set_type(nemopackease0, NEMOEASE_CUBIC_INOUT_TYPE);
	nemopackease1 = nemoshow_ease_create();
	nemoshow_ease_set_type(nemopackease1, NEMOEASE_CUBIC_OUT_TYPE);
}

void __attribute__((destructor(101))) nemopack_finish_envs(void)
{
	nemoshow_one_destroy(nemopackease0);
	nemoshow_one_destroy(nemopackease1);
}

static void nemopack_handle_view_destroy(struct wl_listener *listener, void *data)
{
	struct nemopack *pack = (struct nemopack *)container_of(listener, struct nemopack, view_destroy_listener);

	nemopack_destroy(pack);
}

static void nemopack_handle_bin_resize(struct wl_listener *listener, void *data)
{
	struct nemopack *pack = (struct nemopack *)container_of(listener, struct nemopack, bin_resize_listener);
	struct nemoactor *actor = pack->actor;
	uint32_t width = pack->view->content->width;
	uint32_t height = pack->view->content->height;

	nemoactor_dispatch_resize(actor, width, height, 0);

	nemoshow_dispatch_frame(pack->show);
}

static void nemopack_dispatch_event(struct nemotale *tale, struct talenode *node, uint32_t type, struct taleevent *event)
{
	uint32_t id = nemotale_node_get_id(node);

	if (id == 1) {
		struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
		struct nemoshell *shell = NEMOSHOW_AT(show, shell);
		struct nemoactor *actor = NEMOSHOW_AT(show, actor);
		struct nemocompz *compz = shell->compz;
		struct nemoseat *seat = compz->seat;
		struct nemopack *pack = (struct nemopack *)nemoshow_get_userdata(show);

		if (nemotale_is_touch_down(tale, event, type) ||
				nemotale_is_touch_up(tale, event, type)) {
			nemotale_event_update_node_taps(tale, node, event, type);

			if (nemotale_is_no_tap(tale, event, type)) {
				nemotimer_set_timeout(pack->timer, pack->timeout);
			} else if (nemotale_is_single_tap(tale, event, type)) {
				struct touchpoint *tp;

				tp = nemoseat_get_touchpoint_by_id(seat, event->taps[0]->device);
				if (tp != NULL) {
					nemoshell_move_canvas_by_touchpoint(shell, tp, pack->bin);

					nemotimer_set_timeout(pack->timer, 0);
				}
			} else if (nemotale_is_double_taps(tale, event, type)) {
				struct touchpoint *tp0, *tp1;

				tp0 = nemoseat_get_touchpoint_by_id(seat, event->taps[0]->device);
				tp1 = nemoseat_get_touchpoint_by_id(seat, event->taps[1]->device);
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
			} else if (nemotale_is_many_taps(tale, event, type)) {
				struct touchpoint *tp0, *tp1;

				nemotale_event_update_faraway_taps(tale, event);

				tp0 = nemoseat_get_touchpoint_by_id(seat, event->tap0->device);
				tp1 = nemoseat_get_touchpoint_by_id(seat, event->tap1->device);
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
}

static void nemopack_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct nemopack *pack = (struct nemopack *)data;

	nemopack_destroy(pack);
}

struct nemopack *nemopack_create(struct nemoshell *shell, struct nemoview *view, uint32_t timeout)
{
	struct nemoactor *actor;
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

	pack->show = show = nemoshow_create_actor(shell,
			view->content->width,
			view->content->height,
			nemopack_dispatch_event);
	if (show == NULL)
		goto err2;
	nemoshow_set_userdata(show, pack);

	pack->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, view->content->width);
	nemoshow_scene_set_height(scene, view->content->height);
	nemoshow_attach_one(show, scene);

	pack->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, view->content->width);
	nemoshow_canvas_set_height(canvas, view->content->height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_canvas_set_alpha(canvas, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	pack->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, view->content->width);
	nemoshow_canvas_set_height(canvas, view->content->height);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_event(canvas, 1);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	nemoshow_set_scene(show, scene);
	nemoshow_set_size(show, view->content->width, view->content->height);

	pack->blur0 = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "inner", 5.0f);
	pack->blur1 = blur = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	nemoshow_attach_one(show, blur);
	nemoshow_filter_set_blur(blur, "high", "outer", 0.0f);

	pack->one = one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, view->content->width);
	nemoshow_item_set_height(one, view->content->height);
	nemoshow_item_set_fill_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_item_set_filter(one, pack->blur0);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, one);
	nemoshow_sequence_set_cattr(set0, "fill", 0x1e, 0xdc, 0xdc, 0x40, NEMOSHOW_STYLE_DIRTY);

	pack->icon = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(canvas, one);
	nemoshow_item_set_width(one, view->content->width / 2);
	nemoshow_item_set_height(one, view->content->height / 2);
	nemoshow_item_set_tsr(one);
	nemoshow_item_translate(one, view->content->width / 4, view->content->height / 4);
	nemoshow_item_load_svg(one, NEMOENVS_RESOURCES "/misc-icons/catch.svg");
	nemoshow_item_set_fill_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_item_set_stroke_color(one, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_item_set_stroke_width(one, MIN(view->content->width, view->content->height) / 64);
	nemoshow_item_set_filter(one, pack->blur1);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, one);
	nemoshow_sequence_set_cattr(set1, "fill", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);
	nemoshow_sequence_set_cattr(set1, "stroke", 0x1e, 0xdc, 0xdc, 0xff, NEMOSHOW_STYLE_DIRTY);

	sequence = nemoshow_sequence_create_easy(show,
			nemoshow_sequence_create_frame_easy(show,
				1.0f, set0, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(nemopackease1, 500, 0);
	nemoshow_transition_check_one(trans, one);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_attach_transition(show, trans);

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, pack->blur1);
	nemoshow_sequence_set_dattr(set0, "r", 8.0f, NEMOSHOW_SHAPE_DIRTY);

	set1 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set1, pack->blur1);
	nemoshow_sequence_set_dattr(set1, "r", 3.0f, NEMOSHOW_SHAPE_DIRTY);

	sequence = nemoshow_sequence_create_easy(show,
			nemoshow_sequence_create_frame_easy(show,
				0.5f, set0, NULL),
			nemoshow_sequence_create_frame_easy(show,
				1.0f, set1, NULL),
			NULL);

	trans = nemoshow_transition_create(nemopackease0, 1200, 0);
	nemoshow_transition_check_one(trans, pack->blur1);
	nemoshow_transition_attach_sequence(trans, sequence);
	nemoshow_transition_set_repeat(trans, 0);
	nemoshow_attach_transition(show, trans);

	pack->actor = actor = NEMOSHOW_AT(show, actor);
	nemoview_set_parent(actor->view, view);
	nemoview_set_state(actor->view, NEMO_VIEW_MAPPED_STATE);
	nemoview_update_transform(actor->view);

	nemoshow_dispatch_frame(show);

	pack->view = view;
	pack->bin = nemoshell_get_bin(view->canvas);

	pack->view_destroy_listener.notify = nemopack_handle_view_destroy;
	wl_signal_add(&view->destroy_signal, &pack->view_destroy_listener);

	pack->bin_resize_listener.notify = nemopack_handle_bin_resize;
	wl_signal_add(&pack->bin->resize_signal, &pack->bin_resize_listener);

	nemolist_init(&pack->destroy_listener.link);

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

	nemoshow_revoke_actor(pack->show);
	nemoshow_destroy_actor_on_idle(pack->show);

	free(pack);
}
