#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <nemoshow.h>
#include <showcanvas.h>
#include <showcanvas.hpp>
#include <nemoattr.h>
#include <showmisc.h>
#include <nemomisc.h>
#include <nemolog.h>

void __attribute__((constructor(101))) nemoshow_initialize(void)
{
	SkGraphics::Init();
}

void __attribute__((destructor(101))) nemoshow_finalize(void)
{
	SkGraphics::Term();
}

struct nemoshow *nemoshow_create(void)
{
	struct nemoshow *show;

	show = (struct nemoshow *)malloc(sizeof(struct nemoshow));
	if (show == NULL)
		return NULL;
	memset(show, 0, sizeof(struct nemoshow));

	nemosignal_init(&show->destroy_signal);

	nemolist_init(&show->one_list);
	nemolist_init(&show->dirty_list);
	nemolist_init(&show->bounds_list);
	nemolist_init(&show->redraw_list);
	nemolist_init(&show->transition_list);

	nemolist_init(&show->ptap_list);
	nemolist_init(&show->tap_list);
	nemolist_init(&show->grab_list);

	nemolist_init(&show->scene_destroy_listener.link);

	show->state = NEMOSHOW_ANTIALIAS_STATE | NEMOSHOW_FILTER_STATE;
	show->quality = NEMOSHOW_FILTER_NORMAL_QUALITY;

	show->keyboard.focus = NULL;
	nemolist_init(&show->keyboard.one_destroy_listener.link);

	show->single_click_duration = 700;
	show->single_click_distance = 30;

	return show;
}

void nemoshow_destroy(struct nemoshow *show)
{
	struct showtransition *trans, *ntrans;

	nemosignal_emit(&show->destroy_signal, show);

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		nemoshow_transition_destroy(trans);
	}

	nemolist_remove(&show->one_list);
	nemolist_remove(&show->dirty_list);
	nemolist_remove(&show->bounds_list);
	nemolist_remove(&show->redraw_list);
	nemolist_remove(&show->transition_list);

	nemolist_remove(&show->ptap_list);
	nemolist_remove(&show->tap_list);
	nemolist_remove(&show->grab_list);

	nemolist_remove(&show->scene_destroy_listener.link);

	nemolist_remove(&show->keyboard.one_destroy_listener.link);

	if (show->name != NULL)
		free(show->name);

	free(show);
}

void nemoshow_set_name(struct nemoshow *show, const char *name)
{
	if (show->name != NULL)
		free(show->name);

	show->name = strdup(name);
}

struct showone *nemoshow_search_one(struct nemoshow *show, const char *id)
{
	struct showone *one;

	if (id == NULL || id[0] == '\0')
		return NULL;

	nemoshow_for_each(one, show) {
		if (strcmp(one->id, id) == 0)
			return one;
	}

	return NULL;
}

void nemoshow_enter_frame(struct nemoshow *show, uint32_t msecs)
{
	show->frame_depth++;

	if (show->enter_frame != NULL)
		show->enter_frame(show, msecs);
}

void nemoshow_leave_frame(struct nemoshow *show, uint32_t msecs)
{
	show->frame_depth--;

	if (show->leave_frame != NULL)
		show->leave_frame(show, msecs);
}

int nemoshow_update_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showone *one, *none;

	if (scene == NULL)
		return 0;

	nemolist_for_each_safe(one, none, &show->dirty_list, dirty_link) {
		nemoshow_one_update(one);
	}

	nemolist_for_each_safe(one, none, &show->bounds_list, bounds_link) {
		nemoshow_one_update_bounds(one);
	}

	show->dirty_serial = 0;

	return nemolist_empty(&show->redraw_list) == 0;
}

void nemoshow_render_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showcanvas *canvas, *ncanvas;

	if (scene == NULL)
		return;

	nemolist_for_each_safe(canvas, ncanvas, &show->redraw_list, redraw_link) {
		if (nemoshow_canvas_has_state(canvas, NEMOSHOW_CANVAS_REDRAW_STATE)) {
			nemoshow_clear_time(show);

			canvas->prepare_render(show, NEMOSHOW_CANVAS_ONE(canvas));

			nemoshow_check_time(show, NEMOSHOW_FRAME_PREPARE_TIME);

			canvas->dispatch_redraw(show, NEMOSHOW_CANVAS_ONE(canvas));

			nemoshow_check_time(show, NEMOSHOW_FRAME_RENDER_TIME);

			canvas->finish_render(show, NEMOSHOW_CANVAS_ONE(canvas));

			nemoshow_check_time(show, NEMOSHOW_FRAME_FINISH_TIME);
		}

		nemolist_remove(&canvas->redraw_link);
		nemolist_init(&canvas->redraw_link);
	}
}

static void nemoshow_handle_scene_destroy_signal(struct nemolistener *listener, void *data)
{
	struct nemoshow *show = (struct nemoshow *)container_of(listener, struct nemoshow, scene_destroy_listener);

	show->scene = NULL;

	nemolist_remove(&show->scene_destroy_listener.link);
	nemolist_init(&show->scene_destroy_listener.link);
}

int nemoshow_set_scene(struct nemoshow *show, struct showone *one)
{
	struct showone *child;

	if (show->scene == one)
		return 0;

	if (show->scene != NULL)
		nemoshow_put_scene(show);

	show->scene = one;

	show->scene_destroy_listener.notify = nemoshow_handle_scene_destroy_signal;
	nemosignal_add(&one->destroy_signal, &show->scene_destroy_listener);

	nemoshow_attach_ones(show, one);

	nemoshow_one_dirty(one, NEMOSHOW_SIZE_DIRTY | NEMOSHOW_CHILDREN_DIRTY);

	return 0;
}

void nemoshow_put_scene(struct nemoshow *show)
{
	if (show->scene == NULL)
		return;

	nemoshow_detach_ones(show->scene);

	show->scene = NULL;

	nemolist_remove(&show->scene_destroy_listener.link);
	nemolist_init(&show->scene_destroy_listener.link);
}

int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height)
{
	struct showone *one;
	struct showone *child;

	if (show->width == width && show->height == height)
		return 0;

	show->width = width;
	show->height = height;

	nemotale_resize(show->tale, show->width, show->height);

	if (show->scene != NULL)
		nemoshow_one_dirty(show->scene, NEMOSHOW_VIEWPORT_DIRTY);

	return 1;
}

struct showone *nemoshow_pick_canvas(struct nemoshow *show, float x, float y, float *sx, float *sy)
{
	struct showone *scene = show->scene;
	struct showone *one;

	nemoshow_children_for_each_reverse(one, scene) {
		if (NEMOSHOW_CANVAS_AT(one, dispatch_event) != NULL) {
			struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

			nemoshow_canvas_transform_from_global(one, x, y, sx, sy);

			if (0.0f <= *sx && *sx <= canvas->width && 0.0f <= *sy && *sy <= canvas->height)
				return one;
		}
	}

	return NULL;
}

int nemoshow_contain_canvas(struct nemoshow *show, struct showone *one, float x, float y, float *sx, float *sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemoshow_canvas_transform_from_global(one, x, y, sx, sy);

	if (canvas->contain_point != NULL)
		return canvas->contain_point(show, one, *sx, *sy);

	return (0.0f <= *sx && *sx <= canvas->width && 0.0f <= *sy && *sy <= canvas->height);
}

void nemoshow_attach_one(struct nemoshow *show, struct showone *one)
{
	nemolist_insert_tail(&show->one_list, &one->link);
	one->show = show;

	nemoshow_one_dirty(one, NEMOSHOW_ALL_DIRTY);
}

void nemoshow_detach_one(struct showone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);

	one->show = NULL;
}

void nemoshow_attach_ones(struct nemoshow *show, struct showone *one)
{
	struct showone *child;
	int i;

	if (one->show == show)
		return;

	if (one->show != NULL)
		nemoshow_detach_one(one);

	nemoshow_attach_one(show, one);

	nemoshow_children_for_each(child, one) {
		nemoshow_attach_ones(show, child);
	}

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		if (one->refs[i] != NULL)
			nemoshow_attach_ones(show, one->refs[i]->src);
	}
}

void nemoshow_detach_ones(struct showone *one)
{
	struct showone *child;

	nemoshow_detach_one(one);

	nemoshow_children_for_each(child, one) {
		nemoshow_detach_ones(child);
	}
}

void nemoshow_attach_transition(struct nemoshow *show, struct showtransition *trans)
{
	int i;

	trans->serial = ++show->transition_serial;

	for (i = 0; i < trans->nsequences; i++) {
		nemoshow_sequence_prepare(trans->sequences[i], trans->serial);
	}

	nemolist_insert(&show->transition_list, &trans->link);
}

void nemoshow_detach_transition(struct nemoshow *show, struct showtransition *trans)
{
	nemolist_remove(&trans->link);
	nemolist_init(&trans->link);
}

int nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs)
{
	struct showtransition *trans, *ntrans;
	int has_transition = nemolist_empty(&show->transition_list) == 0;
	int done, i;

	if (nemoshow_has_state(show, NEMOSHOW_ONTIME_STATE))
		msecs = time_current_msecs();

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		done = nemoshow_transition_dispatch(trans, msecs);
		if (done != 0) {
			if (trans->dispatch_done != NULL)
				trans->dispatch_done(trans->userdata);

			if (trans->done != 0 || trans->repeat == 1) {
				nemoshow_transition_destroy(trans);
			} else if (trans->repeat == 0 || --trans->repeat) {
				trans->stime = 0;
				trans->delay = 0;

				for (i = 0; i < trans->nsequences; i++) {
					nemoshow_sequence_prepare(trans->sequences[i], trans->serial);
				}
			}
		}
	}

	return has_transition;
}

int nemoshow_has_transition(struct nemoshow *show)
{
	return nemolist_empty(&show->transition_list) == 0;
}

void nemoshow_ready_transition(struct nemoshow *show, uint32_t msecs)
{
	struct showtransition *trans;

	nemolist_for_each(trans, &show->transition_list, link) {
		if (trans->stime == 0) {
			trans->stime = msecs + trans->delay;
			trans->etime = msecs + trans->delay + trans->duration;
		}
	}
}

struct showtransition *nemoshow_get_last_transition_one(struct nemoshow *show, struct showone *one, const char *name)
{
	struct showtransition *ltrans = NULL;
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *child;
	struct showset *set;
	struct showact *act;
	struct nemoattr *attr;
	int i;

	attr = nemoobject_get(&one->object, name);
	if (attr == NULL)
		return NULL;

	nemolist_for_each(trans, &show->transition_list, link) {
		for (i = 0; i < trans->nsequences; i++) {
			sequence = trans->sequences[i];

			nemoshow_children_for_each(frame, sequence) {
				nemoshow_children_for_each(child, frame) {
					set = NEMOSHOW_SET(child);

					if (set->src == one) {
						nemolist_for_each(act, &set->act_list, link) {
							if (act->attr == attr) {
								if (ltrans == NULL || ltrans->etime < trans->etime)
									ltrans = trans;
							}
						}
					}
				}
			}
		}
	}

	return ltrans;
}

struct showtransition *nemoshow_get_last_transition_tag(struct nemoshow *show, uint32_t tag)
{
	struct showtransition *ltrans = NULL;
	struct showtransition *trans;

	nemolist_for_each(trans, &show->transition_list, link) {
		if (trans->tag == tag) {
			if (ltrans == NULL || ltrans->etime < trans->etime)
				ltrans = trans;
		}
	}

	return ltrans;
}

struct showtransition *nemoshow_get_last_transition_all(struct nemoshow *show)
{
	struct showtransition *ltrans = NULL;
	struct showtransition *trans;

	nemolist_for_each(trans, &show->transition_list, link) {
		if (ltrans == NULL || ltrans->etime < trans->etime)
			ltrans = trans;
	}

	return ltrans;
}

void nemoshow_revoke_transition_one(struct nemoshow *show, struct showone *one, const char *name)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *child;
	struct showset *set;
	struct showact *act, *nact;
	struct nemoattr *attr;
	int i;

	attr = nemoobject_get(&one->object, name);
	if (attr == NULL)
		return;

	nemolist_for_each(trans, &show->transition_list, link) {
		for (i = 0; i < trans->nsequences; i++) {
			sequence = trans->sequences[i];

			nemoshow_children_for_each(frame, sequence) {
				nemoshow_children_for_each(child, frame) {
					set = NEMOSHOW_SET(child);

					if (set->src == one) {
						nemolist_for_each_safe(act, nact, &set->act_list, link) {
							if (act->attr == attr) {
								nemolist_remove(&act->link);

								free(act);
							}
						}
					}
				}
			}
		}
	}
}

void nemoshow_revoke_transition_tag(struct nemoshow *show, uint32_t tag)
{
	struct showtransition *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		if (trans->tag == tag)
			nemoshow_transition_destroy(trans);
	}
}

void nemoshow_revoke_transition_all(struct nemoshow *show)
{
	struct showtransition *trans, *ntrans;

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		nemoshow_transition_destroy(trans);
	}
}

static void nemoshow_handle_keyboard_focus_destroy(struct nemolistener *listener, void *data)
{
	struct nemoshow *show = (struct nemoshow *)container_of(listener, struct nemoshow, keyboard.one_destroy_listener);

	nemolist_remove(&show->keyboard.one_destroy_listener.link);
	nemolist_init(&show->keyboard.one_destroy_listener.link);

	show->keyboard.focus = NULL;
}

void nemoshow_set_keyboard_focus(struct nemoshow *show, struct showone *one)
{
	if (show->keyboard.focus != NULL) {
		nemolist_remove(&show->keyboard.one_destroy_listener.link);
		nemolist_init(&show->keyboard.one_destroy_listener.link);
	}

	show->keyboard.focus = one;

	if (one != NULL) {
		show->keyboard.one_destroy_listener.notify = nemoshow_handle_keyboard_focus_destroy;
		nemosignal_add(&one->destroy_signal, &show->keyboard.one_destroy_listener);
	}
}

void nemoshow_enable_antialias(struct nemoshow *show)
{
	struct showone *one;

	if (nemoshow_has_state(show, NEMOSHOW_ANTIALIAS_STATE) != 0)
		return;
	nemoshow_set_state(show, NEMOSHOW_ANTIALIAS_STATE);

	nemolist_for_each(one, &show->one_list, link) {
		nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
	}
}

void nemoshow_disable_antialias(struct nemoshow *show)
{
	struct showone *one;

	if (nemoshow_has_state(show, NEMOSHOW_ANTIALIAS_STATE) == 0)
		return;
	nemoshow_put_state(show, NEMOSHOW_ANTIALIAS_STATE);

	nemolist_for_each(one, &show->one_list, link) {
		nemoshow_one_dirty(one, NEMOSHOW_STYLE_DIRTY);
	}
}

void nemoshow_enable_filtering(struct nemoshow *show)
{
	struct showone *one;

	if (nemoshow_has_state(show, NEMOSHOW_FILTER_STATE) != 0)
		return;
	nemoshow_set_state(show, NEMOSHOW_FILTER_STATE);

	nemolist_for_each(one, &show->one_list, link) {
		nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
	}
}

void nemoshow_disable_filtering(struct nemoshow *show)
{
	struct showone *one;

	if (nemoshow_has_state(show, NEMOSHOW_FILTER_STATE) == 0)
		return;
	nemoshow_put_state(show, NEMOSHOW_FILTER_STATE);

	nemolist_for_each(one, &show->one_list, link) {
		nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
	}
}

void nemoshow_set_filtering_quality(struct nemoshow *show, uint32_t quality)
{
	struct showone *one;

	if (show->quality == quality)
		return;
	show->quality = quality;

	nemolist_for_each(one, &show->one_list, link) {
		nemoshow_one_dirty(one, NEMOSHOW_FILTER_DIRTY);
	}
}

#ifdef NEMOSHOW_FRAMELOG_ON
void nemoshow_check_damage(struct nemoshow *show)
{
	struct showcanvas *canvas;

	nemolist_for_each(canvas, &show->redraw_list, redraw_link) {
		if (nemoshow_canvas_is_type(canvas, NEMOSHOW_CANVAS_VECTOR_TYPE) != 0) {
			if (nemoshow_canvas_has_state(canvas, NEMOSHOW_CANVAS_REDRAW_FULL_STATE)) {
				show->damages += (canvas->viewport.width * canvas->viewport.height);
			} else {
				SkRegion::Iterator iter(*NEMOSHOW_CANVAS_CC(canvas, damage));

				show->damages += (iter.rect().width() * iter.rect().height());
			}
		}
	}
}

void nemoshow_dump_times(struct nemoshow *show)
{
	static const char *names[] = {
		"FRAME_PREPARE",
		"FRAME_RENDER ",
		"FRAME_FINISH "
	};
	int i;

	if (show->frames == 0)
		return;

	nemolog_message("SHOW", "[%s:%d] size(%dx%d) frames(%d) damages(%d/%f)\n",
			show->name != NULL ? show->name : "noname",
			getpid(),
			show->width,
			show->height,
			show->frames,
			show->damages,
			show->frames > 0 ? (double)show->damages / 1024.0f / 1024.0f / show->frames : 0.0f);

	for (i = 0; i < NEMOSHOW_LAST_TIME; i++)
		nemolog_message("SHOW", "  %s: times(%d-%d)\n", names[i], show->times[i], show->times[i] / show->frames);

	show->frames = 0;
	show->damages = 0;

	for (i = 0; i < NEMOSHOW_LAST_TIME; i++)
		show->times[i] = 0;
}
#endif
