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
#include <touch.h>
#include <move.h>
#include <pick.h>
#include <picker.h>
#include <sound.h>

#include <nemospeaker.h>
#include <nemoshell.h>
#include <showhelper.h>
#include <nemolog.h>
#include <nemomisc.h>

#define NEMOSPEAKER_WIDTH			(512)
#define NEMOSPEAKER_HEIGHT		(512)
#define NEMOSPEAKER_OUTSET		(32)

#define NEMOSPEAKER_CHECK(x, y, x0, y0, x1, y1)	\
	(x0 + NEMOSPEAKER_OUTSET <= x && x < x1 + NEMOSPEAKER_OUTSET && y0 + NEMOSPEAKER_OUTSET <= y && y < y1 + NEMOSPEAKER_OUTSET)

static struct showone *nemospeakerease;

void __attribute__((constructor(101))) nemospeaker_prepare_envs(void)
{
	nemospeakerease = nemoshow_ease_create();
	nemoshow_ease_set_type(nemospeakerease, NEMOEASE_CUBIC_INOUT_TYPE);
}

void __attribute__((destructor(101))) nemospeaker_finish_envs(void)
{
	nemoshow_one_destroy(nemospeakerease);
}

static int nemospeaker_dispatch_volume_handle_rotate_grab(struct nemoshow *show, struct showgrab *grab, void *event)
{
	struct nemospeaker *speaker = (struct nemospeaker *)nemoshow_grab_get_userdata(grab);

	if (nemoshow_event_is_down(show, event)) {
	} else if (nemoshow_event_is_motion(show, event)) {
		float gx = nemoshow_event_get_grab_x(event);
		float x = nemoshow_event_get_x(event);
		double dv = (x - gx) / 396.0f;
		double nv = MAX(MIN(speaker->volumeratio + dv, 1.0f), 0.0f);

		nemoshow_item_set_width(speaker->volume, nv * 396.0f);

		nemoshow_dispatch_frame(speaker->show);

		if (speaker->pid != 0) {
			nemosound_set_volume(speaker->shell->compz->sound, speaker->pid, (uint32_t)(nv * 100.0f));
		} else if (speaker->sink != NULL) {
			nemosound_set_volume_sink(speaker->shell->compz->sound, speaker->sink->id, (uint32_t)(nv * 100.0f));
		}
	} else if (nemoshow_event_is_up(show, event)) {
		if (speaker->pid != 0) {
			nemosound_get_info(speaker->shell->compz->sound, speaker->pid);
		} else if (speaker->sink != NULL) {
			nemosound_get_info_sink(speaker->shell->compz->sound, speaker->sink->id);
		}

		nemoshow_grab_destroy(grab);

		return 0;
	}

	return 1;
}

static void nemospeaker_dispatch_canvas_event(struct nemoshow *show, struct showone *canvas, void *event)
{
	struct nemospeaker *speaker = (struct nemospeaker *)nemoshow_get_userdata(show);
	struct nemocompz *compz = speaker->shell->compz;

	if (nemoshow_event_is_down(show, event)) {
		if (NEMOSPEAKER_CHECK(nemoshow_event_get_x(event), nemoshow_event_get_y(event), 58, 226, 454, 286)) {
			struct showgrab *grab;

			grab = nemoshow_grab_create(show, event, nemospeaker_dispatch_volume_handle_rotate_grab);
			nemoshow_grab_set_userdata(grab, speaker);
			nemoshow_grab_check_signal(grab, &speaker->destroy_signal);
			nemoshow_dispatch_grab(show, event);
		} else {
			nemoshow_event_set_tag(event, 1);
		}
	} else if (nemoshow_event_is_up(show, event)) {
		struct nemoview *view;
		struct wl_client *client;
		float sx, sy;

		view = nemocompz_pick_canvas(compz, nemoshow_event_get_gx(event), nemoshow_event_get_gy(event), &sx, &sy);
		if ((view != NULL) &&
				(nemoview_has_state(view, NEMO_VIEW_SOUND_STATE)) &&
				(view->canvas != NULL) &&
				(view->canvas->resource != NULL) &&
				(client = wl_resource_get_client(view->canvas->resource)) != NULL) {
			pid_t pid;

			wl_client_get_credentials(client, &pid, NULL, NULL);

			speaker->pid = pid;

			if (speaker->sink != NULL)
				nemosound_set_sink(compz->sound, speaker->pid, speaker->sink->id);

			nemosound_get_info(compz->sound, speaker->pid);
		} else if (speaker->sink != NULL) {
			speaker->pid = 0;

			nemosound_get_info_sink(compz->sound, speaker->sink->id);
		}
	}

	if (nemoshow_event_is_down(show, event) || nemoshow_event_is_up(show, event)) {
		nemoshow_event_update_taps_by_tag(show, event, 1);

		if (nemoshow_event_is_single_tap(show, event)) {
			nemoshow_view_move(show, nemoshow_event_get_device_on(event, 0));
		} else if (nemoshow_event_is_many_taps(show, event)) {
			nemoshow_view_put_pivot(show);

			nemoshow_view_pick_distant(show, event, NEMOSHOW_VIEW_PICK_ROTATE_TYPE | NEMOSHOW_VIEW_PICK_SCALE_TYPE | NEMOSHOW_VIEW_PICK_TRANSLATE_TYPE);
		}
	}
}

static void nemospeaker_dispatch_show_destroy(struct nemoshow *show)
{
	struct nemospeaker *speaker = (struct nemospeaker *)nemoshow_get_userdata(show);

	nemospeaker_destroy(speaker);
}

static void nemospeaker_dispatch_fadein_transition(struct nemoshow *show, struct showone *one, struct showone *ease, uint32_t duration, uint32_t delay)
{
	struct showtransition *trans;
	struct showone *set0;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, one);
	nemoshow_sequence_set_dattr(set0, "alpha", 1.0f, NEMOSHOW_STYLE_DIRTY);

	trans = nemoshow_transition_create(ease, duration, delay);
	nemoshow_transition_check_one(trans, one);
	nemoshow_transition_attach_sequence(trans,
			nemoshow_sequence_create_easy(show,
				nemoshow_sequence_create_frame_easy(show,
					1.0f, set0, NULL),
				NULL));
	nemoshow_attach_transition(show, trans);
}

static void nemospeaker_handle_sound_info(struct wl_listener *listener, void *data)
{
	struct nemospeaker *speaker = (struct nemospeaker *)container_of(listener, struct nemospeaker, sound_info_listener);
	struct soundinfo *info = (struct soundinfo *)data;

	if (info->is_sink == 0) {
		if (speaker->pid != 0 && speaker->pid == info->id) {
			nemospeaker_set_volume(speaker, info->volume, 300, 0);
		}
	} else {
		if (speaker->pid == 0 && speaker->sink != NULL && speaker->sink->id == info->id) {
			nemospeaker_set_volume(speaker, info->volume, 300, 0);
		}
	}
}

struct nemospeaker *nemospeaker_create(struct nemoshell *shell, uint32_t size, double x, double y, double r)
{
	struct nemocompz *compz = shell->compz;
	struct nemospeaker *speaker;
	struct nemoshow *show;
	struct showone *scene;
	struct showone *canvas;
	struct showone *group;
	struct showone *one;
	int32_t width = NEMOSPEAKER_WIDTH;
	int32_t height = NEMOSPEAKER_HEIGHT;

	speaker = (struct nemospeaker *)malloc(sizeof(struct nemospeaker));
	if (speaker == NULL)
		return NULL;
	memset(speaker, 0, sizeof(struct nemospeaker));

	speaker->shell = shell;

	speaker->x = x;
	speaker->y = y;
	speaker->r = r;

	speaker->volumeratio = 0.0f;

	nemosignal_init(&speaker->destroy_signal);

	speaker->show = show = nemoshow_create_view(shell, size, size);
	if (show == NULL)
		goto err1;
	nemoshow_set_dispatch_destroy(show, nemospeaker_dispatch_show_destroy);
	nemoshow_set_userdata(show, speaker);

	speaker->scene = scene = nemoshow_scene_create();
	nemoshow_scene_set_width(scene, width + NEMOSPEAKER_OUTSET * 2);
	nemoshow_scene_set_height(scene, height + NEMOSPEAKER_OUTSET * 2);
	nemoshow_attach_one(show, scene);

	speaker->back = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width + NEMOSPEAKER_OUTSET * 2);
	nemoshow_canvas_set_height(canvas, height + NEMOSPEAKER_OUTSET * 2);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_BACK_TYPE);
	nemoshow_canvas_set_fill_color(canvas, 0.0f, 0.0f, 0.0f, 0.0f);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	speaker->canvas = canvas = nemoshow_canvas_create();
	nemoshow_canvas_set_width(canvas, width + NEMOSPEAKER_OUTSET * 2);
	nemoshow_canvas_set_height(canvas, height + NEMOSPEAKER_OUTSET * 2);
	nemoshow_canvas_set_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE);
	nemoshow_canvas_set_dispatch_event(canvas, nemospeaker_dispatch_canvas_event);
	nemoshow_attach_one(show, canvas);
	nemoshow_one_attach(scene, canvas);

	speaker->group = group = nemoshow_item_create(NEMOSHOW_GROUP_ITEM);
	nemoshow_attach_one(show, group);
	nemoshow_one_attach(canvas, group);
	nemoshow_item_set_tsr(group);
	nemoshow_item_translate(group, NEMOSPEAKER_OUTSET, NEMOSPEAKER_OUTSET);

	speaker->layout0 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(group, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0x40);
	nemoshow_item_set_alpha(one, 0.0f);
	nemoshow_item_load_svg(one, NEMOENVS_RESOURCES "/speaker/speaker-1.svg");

	speaker->layout1 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(group, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);
	nemoshow_item_set_fill_color(one, 0x1e, 0xdc, 0xdc, 0xff);
	nemoshow_item_set_alpha(one, 0.0f);
	nemoshow_item_load_svg(one, NEMOENVS_RESOURCES "/speaker/speaker-2.svg");

	speaker->layout2 = one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(group, one);
	nemoshow_item_set_x(one, 0.0f);
	nemoshow_item_set_y(one, 0.0f);
	nemoshow_item_set_width(one, width);
	nemoshow_item_set_height(one, height);
	nemoshow_item_set_fill_color(one, 0xff, 0x8c, 0x32, 0xff);
	nemoshow_item_set_alpha(one, 0.0f);
	nemoshow_item_load_svg(one, NEMOENVS_RESOURCES "/speaker/speaker-3.svg");

	speaker->volume = one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	nemoshow_attach_one(show, one);
	nemoshow_one_attach(group, one);
	nemoshow_item_set_x(one, 58.0f);
	nemoshow_item_set_y(one, 226.0f);
	nemoshow_item_set_width(one, 0.0f);
	nemoshow_item_set_height(one, 60.0f);
	nemoshow_item_set_fill_color(one, 0xff, 0x8c, 0x32, 0xff);
	nemoshow_item_set_alpha(one, 0.0f);

	nemoshow_render_one(show);

	nemoshow_view_set_layer(show, "overlay");
	nemoshow_view_set_position(show,
			speaker->x - size / 2.0f,
			speaker->y - size / 2.0f);
	nemoshow_view_set_pivot(show, size / 2.0f, size / 2.0f);
	nemoshow_view_set_rotation(show, speaker->r);

	nemospeaker_dispatch_fadein_transition(show, speaker->layout0, nemospeakerease, 1500, 0);
	nemospeaker_dispatch_fadein_transition(show, speaker->layout1, nemospeakerease, 1000, 250);
	nemospeaker_dispatch_fadein_transition(show, speaker->layout2, nemospeakerease, 1000, 500);
	nemospeaker_dispatch_fadein_transition(show, speaker->volume, nemospeakerease, 1000, 750);

	nemoshow_dispatch_frame(show);

	speaker->sound_info_listener.notify = nemospeaker_handle_sound_info;
	wl_signal_add(&compz->sound->info_signal, &speaker->sound_info_listener);

	return speaker;

err1:
	free(speaker);

	return NULL;
}

void nemospeaker_destroy(struct nemospeaker *speaker)
{
	nemosignal_emit(&speaker->destroy_signal, speaker);

	wl_list_remove(&speaker->sound_info_listener.link);

	nemoshow_one_destroy(speaker->layout0);
	nemoshow_one_destroy(speaker->layout1);
	nemoshow_one_destroy(speaker->layout2);
	nemoshow_one_destroy(speaker->volume);

	nemoshow_put_scene(speaker->show);

	nemoshow_revoke_view(speaker->show);
	nemoshow_destroy_view_on_idle(speaker->show);

	free(speaker);
}

void nemospeaker_set_sink(struct nemospeaker *speaker, struct soundsink *sink)
{
	speaker->sink = sink;
}

void nemospeaker_set_volume(struct nemospeaker *speaker, uint32_t volume, uint32_t duration, uint32_t delay)
{
	struct showtransition *trans;
	struct showone *set0;

	set0 = nemoshow_sequence_create_set();
	nemoshow_sequence_set_source(set0, speaker->volume);
	nemoshow_sequence_set_dattr(set0, "width", ((double)volume / 100.0f) * 396.0f, NEMOSHOW_SHAPE_DIRTY);

	trans = nemoshow_transition_create(nemospeakerease, duration, delay);
	nemoshow_transition_check_one(trans, speaker->volume);
	nemoshow_transition_attach_sequence(trans,
			nemoshow_sequence_create_easy(speaker->show,
				nemoshow_sequence_create_frame_easy(speaker->show,
					1.0f, set0, NULL),
				NULL));
	nemoshow_attach_transition(speaker->show, trans);

	nemoshow_dispatch_frame(speaker->show);

	speaker->volumeratio = (double)volume / 100.0f;
}
