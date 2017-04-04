#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemomotz.h>
#include <nemomisc.h>

struct nemomotz *nemomotz_create(void)
{
	struct nemomotz *motz;

	motz = (struct nemomotz *)malloc(sizeof(struct nemomotz));
	if (motz == NULL)
		return NULL;
	memset(motz, 0, sizeof(struct nemomotz));

	motz->tozz = nemotozz_create();
	if (motz->tozz == NULL)
		goto err1;

	nemolist_init(&motz->one_list);
	nemolist_init(&motz->tap_list);

	motz->transitions = nemotransition_group_create();

	motz->down = nemomotz_down_null;
	motz->motion = nemomotz_motion_null;
	motz->up = nemomotz_up_null;

	return motz;

err1:
	free(motz);

	return NULL;
}

void nemomotz_destroy(struct nemomotz *motz)
{
	nemolist_remove(&motz->one_list);
	nemolist_remove(&motz->tap_list);

	nemotransition_group_destroy(motz->transitions);

	nemotozz_destroy(motz->tozz);

	free(motz);
}

void nemomotz_set_size(struct nemomotz *motz, int width, int height)
{
	motz->width = width;
	motz->height = height;
}

static inline void nemomotz_update_one(struct nemomotz *motz, struct motzone *one, uint32_t msecs)
{
	struct motzone *cone, *none;

	if (nemomotz_one_has_no_dirty(one) == 0) {
		nemomotz_one_update(one);

		nemomotz_set_flags(motz, NEMOMOTZ_REDRAW_FLAG);
	}

	nemolist_for_each_safe(cone, none, &one->one_list, link) {
		nemomotz_update_one(motz, cone, msecs);
	}

	if (nemomotz_one_frame(one, msecs) != 0)
		nemomotz_set_flags(motz, NEMOMOTZ_REDRAW_FLAG);
}

void nemomotz_update(struct nemomotz *motz, uint32_t msecs)
{
	struct motzone *one, *none;

	if (nemotransition_group_has_transition(motz->transitions) != 0) {
		nemotransition_group_dispatch(motz->transitions, msecs);

		nemomotz_set_flags(motz, NEMOMOTZ_REDRAW_FLAG);
	}

	nemolist_for_each_safe(one, none, &motz->one_list, link) {
		nemomotz_update_one(motz, one, msecs);
	}
}

int nemomotz_attach_buffer(struct nemomotz *motz, void *buffer, int width, int height)
{
	motz->viewport.width = width;
	motz->viewport.height = height;

	return nemotozz_attach_buffer(motz->tozz,
			NEMOTOZZ_CANVAS_RGBA_COLOR,
			NEMOTOZZ_CANVAS_PREMUL_ALPHA,
			buffer,
			width,
			height);
}

void nemomotz_detach_buffer(struct nemomotz *motz)
{
	nemotozz_detach_buffer(motz->tozz);
}

void nemomotz_update_buffer(struct nemomotz *motz)
{
	struct nemotozz *tozz = motz->tozz;
	struct motzone *one;

	nemotozz_save(tozz);

	if (motz->width != motz->viewport.width || motz->height != motz->viewport.height)
		nemotozz_scale(tozz,
				(float)motz->viewport.width / (float)motz->width,
				(float)motz->viewport.height / (float)motz->height);

	nemolist_for_each(one, &motz->one_list, link)
		nemomotz_one_draw(motz, one);

	nemotozz_restore(tozz);
}

void nemomotz_clear_buffer(struct nemomotz *motz)
{
	nemotozz_clear(motz->tozz);
}

void nemomotz_attach_one(struct nemomotz *motz, struct motzone *one)
{
	nemolist_insert_tail(&motz->one_list, &one->link);
}

void nemomotz_detach_one(struct nemomotz *motz, struct motzone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);
}

struct motzone *nemomotz_pick_one(struct nemomotz *motz, float x, float y)
{
	struct motzone *one;
	struct motzone *tone;

	nemolist_for_each_reverse(one, &motz->one_list, link) {
		tone = nemomotz_one_contain(one, x, y);
		if (tone != NULL)
			return tone;
	}

	return NULL;
}

void nemomotz_dispatch_down_event(struct nemomotz *motz, uint64_t id, float x, float y)
{
	struct motztap *tap;
	struct motzone *one;
	float sx, sy;

	nemomotz_transform_from_viewport(motz, x, y, &sx, &sy);

	tap = nemomotz_tap_create();
	if (tap != NULL) {
		nemomotz_tap_set_id(tap, id);
		nemomotz_attach_tap(motz, tap);

		one = nemomotz_pick_one(motz, sx, sy);
		if (one != NULL) {
			nemomotz_tap_set_one(tap, one);

			nemomotz_one_down(motz, tap, one, sx, sy);
		} else {
			nemomotz_down(motz, tap, sx, sy);
		}
	}
}

void nemomotz_dispatch_motion_event(struct nemomotz *motz, uint64_t id, float x, float y)
{
	struct motztap *tap;
	float sx, sy;

	nemomotz_transform_from_viewport(motz, x, y, &sx, &sy);

	tap = nemomotz_find_tap(motz, id);
	if (tap != NULL) {
		if (tap->one != NULL)
			nemomotz_one_motion(motz, tap, tap->one, sx, sy);
		else
			nemomotz_motion(motz, tap, sx, sy);
	}
}

void nemomotz_dispatch_up_event(struct nemomotz *motz, uint64_t id, float x, float y)
{
	struct motztap *tap;
	float sx, sy;

	nemomotz_transform_from_viewport(motz, x, y, &sx, &sy);

	tap = nemomotz_find_tap(motz, id);
	if (tap != NULL) {
		if (tap->one != NULL)
			nemomotz_one_up(motz, tap, tap->one, sx, sy);
		else
			nemomotz_up(motz, tap, sx, sy);

		nemomotz_tap_destroy(tap);
	}
}

void nemomotz_down_null(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
}

void nemomotz_motion_null(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
}

void nemomotz_up_null(struct nemomotz *motz, struct motztap *tap, float x, float y)
{
}

void nemomotz_one_draw_null(struct nemomotz *motz, struct motzone *one)
{
}

void nemomotz_one_down_null(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

void nemomotz_one_motion_null(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

void nemomotz_one_up_null(struct nemomotz *motz, struct motztap *tap, struct motzone *one, float x, float y)
{
}

struct motzone *nemomotz_one_contain_null(struct motzone *one, float x, float y)
{
	return NULL;
}

void nemomotz_one_update_null(struct motzone *one)
{
}

int nemomotz_one_frame_null(struct motzone *one, uint32_t msecs)
{
	return 0;
}

void nemomotz_one_destroy_null(struct motzone *one)
{
	nemosignal_emit(&one->destroy_signal, one);

	nemolist_remove(&one->link);
	nemolist_remove(&one->one_list);

	free(one);
}

struct motzone *nemomotz_one_create(void)
{
	struct motzone *one;

	one = (struct motzone *)malloc(sizeof(struct motzone));
	if (one == NULL)
		return NULL;
	memset(one, 0, sizeof(struct motzone));

	nemolist_init(&one->link);
	nemolist_init(&one->one_list);

	nemosignal_init(&one->destroy_signal);

	one->draw = nemomotz_one_draw_null;
	one->down = nemomotz_one_down_null;
	one->motion = nemomotz_one_motion_null;
	one->up = nemomotz_one_up_null;
	one->contain = nemomotz_one_contain_null;
	one->update = nemomotz_one_update_null;
	one->frame = nemomotz_one_frame_null;
	one->destroy = nemomotz_one_destroy_null;

	return one;
}

int nemomotz_one_prepare(struct motzone *one)
{
	nemolist_init(&one->link);
	nemolist_init(&one->one_list);

	nemosignal_init(&one->destroy_signal);

	one->draw = nemomotz_one_draw_null;
	one->down = nemomotz_one_down_null;
	one->motion = nemomotz_one_motion_null;
	one->up = nemomotz_one_up_null;
	one->contain = nemomotz_one_contain_null;
	one->update = nemomotz_one_update_null;
	one->frame = nemomotz_one_frame_null;
	one->destroy = nemomotz_one_destroy_null;

	return 0;
}

void nemomotz_one_finish(struct motzone *one)
{
	nemosignal_emit(&one->destroy_signal, one);

	nemolist_remove(&one->link);
	nemolist_remove(&one->one_list);
}

void nemomotz_one_attach_one(struct motzone *one, struct motzone *child)
{
	nemolist_insert_tail(&one->one_list, &child->link);
}

void nemomotz_one_detach_one(struct motzone *one, struct motzone *child)
{
	nemolist_remove(&child->link);
	nemolist_init(&child->link);
}

struct motztap *nemomotz_tap_create(void)
{
	struct motztap *tap;

	tap = (struct motztap *)malloc(sizeof(struct motztap));
	if (tap == NULL)
		return NULL;
	memset(tap, 0, sizeof(struct motztap));

	nemolist_init(&tap->link);
	nemolist_init(&tap->destroy_listener.link);

	return tap;
}

void nemomotz_tap_destroy(struct motztap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_remove(&tap->destroy_listener.link);

	free(tap);
}

void nemomotz_tap_set_id(struct motztap *tap, uint64_t id)
{
	tap->id = id;
}

static void nemomotz_tap_handle_one_destroy(struct nemolistener *listener, void *data)
{
	struct motztap *tap = (struct motztap *)container_of(listener, struct motztap, destroy_listener);

	nemolist_remove(&tap->destroy_listener.link);
	nemolist_init(&tap->destroy_listener.link);

	tap->one = NULL;
}

void nemomotz_tap_set_one(struct motztap *tap, struct motzone *one)
{
	if (tap->one != NULL)
		nemolist_remove(&tap->destroy_listener.link);

	tap->one = one;

	tap->destroy_listener.notify = nemomotz_tap_handle_one_destroy;
	nemosignal_add(&one->destroy_signal, &tap->destroy_listener);
}

void nemomotz_attach_tap(struct nemomotz *motz, struct motztap *tap)
{
	nemolist_insert_tail(&motz->tap_list, &tap->link);
}

void nemomotz_detach_tap(struct nemomotz *motz, struct motztap *tap)
{
	nemolist_remove(&tap->link);
	nemolist_init(&tap->link);
}

struct motztap *nemomotz_find_tap(struct nemomotz *motz, uint64_t id)
{
	struct motztap *tap;

	nemolist_for_each(tap, &motz->tap_list, link) {
		if (tap->id == id)
			return tap;
	}

	return NULL;
}

void nemomotz_attach_transition(struct nemomotz *motz, struct nemotransition *trans)
{
	nemotransition_group_attach_transition(motz->transitions, trans);
}

void nemomotz_detach_transition(struct nemomotz *motz, struct nemotransition *trans)
{
	nemotransition_group_detach_transition(motz->transitions, trans);
}

void nemomotz_revoke_transition(struct nemomotz *motz, void *var, int size)
{
	nemotransition_group_revoke_one(motz->transitions, var, size);
}
