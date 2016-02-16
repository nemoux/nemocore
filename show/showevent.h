#ifndef __NEMOSHOW_EVENT_H__
#define __NEMOSHOW_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoshow.h>
#include <taleevent.h>
#include <talegesture.h>
#include <talegrab.h>

extern int nemoshow_event_dispatch_grab(struct nemoshow *show, void *event, nemotale_grab_dispatch_event_t dispatch, void *data, uint32_t tag, struct nemosignal *signal);

static inline void nemoshow_event_update_taps(struct nemoshow *show, struct showone *one, void *event)
{
	if (one == NULL)
		nemotale_event_update_taps(show->tale, event);
	else
		nemotale_event_update_node_taps(show->tale, NEMOSHOW_CANVAS_AT(one, node), event);
}

static inline void nemoshow_event_update_taps_distant(struct nemoshow *show, struct showone *one, void *event, uint64_t *device0, uint64_t *device1)
{
	nemotale_event_update_taps_distant(show->tale, event, device0, device1);
}

static inline int nemoshow_event_is_single_click(struct nemoshow *show, void *event)
{
	return nemotale_is_single_click(show->tale, event);
}

static inline int nemoshow_event_is_no_tap(struct nemoshow *show, void *event)
{
	return nemotale_is_no_tap(show->tale, event);
}

static inline int nemoshow_event_is_single_tap(struct nemoshow *show, void *event)
{
	return nemotale_is_single_tap(show->tale, event);
}

static inline int nemoshow_event_is_double_taps(struct nemoshow *show, void *event)
{
	return nemotale_is_double_taps(show->tale, event);
}

static inline int nemoshow_event_is_triple_taps(struct nemoshow *show, void *event)
{
	return nemotale_is_triple_taps(show->tale, event);
}

static inline int nemoshow_event_is_many_taps(struct nemoshow *show, void *event)
{
	return nemotale_is_many_taps(show->tale, event);
}

static inline int nemoshow_event_is_down_event(struct nemoshow *show, void *event)
{
	return nemotale_is_down_event(show->tale, event);
}

static inline int nemoshow_event_is_motion_event(struct nemoshow *show, void *event)
{
	return nemotale_is_motion_event(show->tale, event);
}

static inline int nemoshow_event_is_up_event(struct nemoshow *show, void *event)
{
	return nemotale_is_up_event(show->tale, event);
}

static inline int nemoshow_event_is_long_press(struct nemoshow *show, void *event)
{
	return nemotale_is_long_press(show->tale, event);
}

static inline int nemoshow_event_is_pointer_enter(struct nemoshow *show, void *event)
{
	return nemotale_is_pointer_enter(show->tale, event);
}

static inline int nemoshow_event_is_pointer_leave(struct nemoshow *show, void *event)
{
	return nemotale_is_pointer_leave(show->tale, event);
}

static inline int nemoshow_event_is_pointer_button_down(struct nemoshow *show, void *event, uint32_t button)
{
	return nemotale_is_pointer_button_down(show->tale, event, button);
}

static inline int nemoshow_event_is_pointer_button_up(struct nemoshow *show, void *event, uint32_t button)
{
	return nemotale_is_pointer_button_up(show->tale, event, button);
}

static inline int nemoshow_event_is_pointer_axis(struct nemoshow *show, void *event)
{
	return nemotale_is_pointer_axis(show->tale, event);
}

static inline int nemoshow_event_is_keyboard_enter(struct nemoshow *show, void *event)
{
	return nemotale_is_keyboard_enter(show->tale, event);
}

static inline int nemoshow_event_is_keyboard_leave(struct nemoshow *show, void *event)
{
	return nemotale_is_keyboard_leave(show->tale, event);
}

static inline int nemoshow_event_is_keyboard_down(struct nemoshow *show, void *event)
{
	return nemotale_is_keyboard_down(show->tale, event);
}

static inline int nemoshow_event_is_keyboard_up(struct nemoshow *show, void *event)
{
	return nemotale_is_keyboard_up(show->tale, event);
}

static inline int nemoshow_event_is_stick_enter(struct nemoshow *show, void *event)
{
	return nemotale_is_stick_enter(show->tale, event);
}

static inline int nemoshow_event_is_stick_leave(struct nemoshow *show, void *event)
{
	return nemotale_is_stick_leave(show->tale, event);
}

static inline int nemoshow_event_is_stick_translate(struct nemoshow *show, void *event)
{
	return nemotale_is_stick_translate(show->tale, event);
}

static inline int nemoshow_event_is_stick_rotate(struct nemoshow *show, void *event)
{
	return nemotale_is_stick_rotate(show->tale, event);
}

static inline int nemoshow_event_is_stick_button_down(struct nemoshow *show, void *event)
{
	return nemotale_is_stick_button_down(show->tale, event);
}

static inline int nemoshow_event_is_stick_button_up(struct nemoshow *show, void *event)
{
	return nemotale_is_stick_button_up(show->tale, event);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
