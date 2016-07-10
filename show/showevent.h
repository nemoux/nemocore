#ifndef __NEMOSHOW_EVENT_H__
#define __NEMOSHOW_EVENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>
#include <nemolistener.h>

#include <taleevent.h>
#include <talegesture.h>

static inline float nemoshow_event_get_x(void *event)
{
	return nemotale_event_get_x((struct taleevent *)event);
}

static inline float nemoshow_event_get_y(void *event)
{
	return nemotale_event_get_y((struct taleevent *)event);
}

static inline float nemoshow_event_get_z(void *event)
{
	return nemotale_event_get_z((struct taleevent *)event);
}

static inline float nemoshow_event_get_gx(void *event)
{
	return nemotale_event_get_gx((struct taleevent *)event);
}

static inline float nemoshow_event_get_gy(void *event)
{
	return nemotale_event_get_gy((struct taleevent *)event);
}

static inline float nemoshow_event_get_x_on(void *event, int index)
{
	return nemotale_event_get_x_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_y_on(void *event, int index)
{
	return nemotale_event_get_y_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_gx_on(void *event, int index)
{
	return nemotale_event_get_gx_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_gy_on(void *event, int index)
{
	return nemotale_event_get_gy_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_r(void *event)
{
	return nemotale_event_get_r((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_axis(void *event)
{
	return nemotale_event_get_axis((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_serial(void *event)
{
	return nemotale_event_get_serial((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_serial_on(void *event, int index)
{
	return nemotale_event_get_serial_on((struct taleevent *)event, index);
}

static inline uint32_t nemoshow_event_get_time(void *event)
{
	return nemotale_event_get_time((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_value(void *event)
{
	return nemotale_event_get_value((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_duration(void *event)
{
	return nemotale_event_get_duration((struct taleevent *)event);
}

static inline uint64_t nemoshow_event_get_device(void *event)
{
	return nemotale_event_get_device((struct taleevent *)event);
}

static inline uint64_t nemoshow_event_get_device_on(void *event, int index)
{
	return nemotale_event_get_device_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_grab_x(void *event)
{
	return nemotale_event_get_grab_x((struct taleevent *)event);
}

static inline float nemoshow_event_get_grab_y(void *event)
{
	return nemotale_event_get_grab_y((struct taleevent *)event);
}

static inline float nemoshow_event_get_grab_gx(void *event)
{
	return nemotale_event_get_grab_gx((struct taleevent *)event);
}

static inline float nemoshow_event_get_grab_gy(void *event)
{
	return nemotale_event_get_grab_gy((struct taleevent *)event);
}

static inline float nemoshow_event_get_grab_x_on(void *event, int index)
{
	return nemotale_event_get_grab_x_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_grab_y_on(void *event, int index)
{
	return nemotale_event_get_grab_y_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_grab_gx_on(void *event, int index)
{
	return nemotale_event_get_grab_gx_on((struct taleevent *)event, index);
}

static inline float nemoshow_event_get_grab_gy_on(void *event, int index)
{
	return nemotale_event_get_grab_gy_on((struct taleevent *)event, index);
}

static inline uint32_t nemoshow_event_get_grab_time(void *event)
{
	return nemotale_event_get_grab_time((struct taleevent *)event);
}

static inline uint32_t nemoshow_event_get_grab_time_on(void *event, int index)
{
	return nemotale_event_get_grab_time_on((struct taleevent *)event, index);
}

static inline void nemoshow_event_set_tag(void *event, uint32_t tag)
{
	nemotale_event_set_tag((struct taleevent *)event, tag);
}

static inline uint32_t nemoshow_event_get_tag(void *event)
{
	return nemotale_event_get_tag((struct taleevent *)event);
}

static inline const char *nemoshow_event_get_name(void *event)
{
	return nemotale_event_get_name((struct taleevent *)event);
}

static inline void nemoshow_event_set_used(void *event)
{
	nemotale_event_set_used((struct taleevent *)event);
}

static inline void nemoshow_event_set_used_on(void *event, int index)
{
	nemotale_event_set_used_on((struct taleevent *)event, index);
}

static inline void nemoshow_event_set_used_all(void *event)
{
	nemotale_event_set_used_all((struct taleevent *)event);
}

static inline int nemoshow_event_is_used(void *event)
{
	return nemotale_event_is_used((struct taleevent *)event);
}

static inline void nemoshow_event_set_cancel(void *event)
{
	nemotale_event_set_cancel((struct taleevent *)event);
}

static inline int nemoshow_event_get_tapcount(void *event)
{
	return nemotale_event_get_tapcount((struct taleevent *)event);
}

static inline void nemoshow_event_update_taps(struct nemoshow *show, struct showone *one, void *event)
{
	if (one == NULL)
		nemotale_event_update_taps(show->tale, (struct taleevent *)event);
	else
		nemotale_event_update_taps_by_node(show->tale, NEMOSHOW_CANVAS_AT(one, node), (struct taleevent *)event);
}

static inline void nemoshow_event_update_taps_by_tag(struct nemoshow *show, void *event, uint32_t tag)
{
	nemotale_event_update_taps_by_tag(show->tale, (struct taleevent *)event, tag);
}

static inline void nemoshow_event_get_distant_tapdevices(struct nemoshow *show, void *event, uint64_t *device0, uint64_t *device1)
{
	nemotale_event_get_distant_tapdevices(show->tale, (struct taleevent *)event, device0, device1);
}

static inline void nemoshow_event_get_distant_tapserials(struct nemoshow *show, void *event, uint32_t *serial0, uint32_t *serial1)
{
	nemotale_event_get_distant_tapserials(show->tale, (struct taleevent *)event, serial0, serial1);
}

static inline void nemoshow_event_transform_to_viewport(struct nemoshow *show, float x, float y, float *sx, float *sy)
{
	nemotale_event_transform_to_viewport(show->tale, x, y, sx, sy);
}

static inline void nemoshow_event_transform_from_viewport(struct nemoshow *show, float sx, float sy, float *x, float *y)
{
	nemotale_event_transform_from_viewport(show->tale, sx, sy, x, y);
}

static inline int nemoshow_event_is_single_click(struct nemoshow *show, void *event)
{
	return nemotale_event_is_single_click(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_no_tap(struct nemoshow *show, void *event)
{
	return nemotale_event_is_no_tap(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_single_tap(struct nemoshow *show, void *event)
{
	return nemotale_event_is_single_tap(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_double_taps(struct nemoshow *show, void *event)
{
	return nemotale_event_is_double_taps(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_triple_taps(struct nemoshow *show, void *event)
{
	return nemotale_event_is_triple_taps(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_many_taps(struct nemoshow *show, void *event)
{
	return nemotale_event_is_many_taps(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_more_taps(struct nemoshow *show, void *event, int tapcount)
{
	return nemotale_event_is_more_taps(show->tale, (struct taleevent *)event, tapcount);
}

static inline int nemoshow_event_is_no_tap_with_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_no_tap_with_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_single_tap_with_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_single_tap_with_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_double_taps_with_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_double_taps_with_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_triple_taps_with_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_triple_taps_with_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_many_taps_with_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_many_taps_with_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_more_taps_with_up(struct nemoshow *show, void *event, int tapcount)
{
	return nemotale_event_is_more_taps_with_up(show->tale, (struct taleevent *)event, tapcount);
}

static inline int nemoshow_event_is_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_motion(struct nemoshow *show, void *event)
{
	return nemotale_event_is_motion(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_cancel(struct nemoshow *show, void *event)
{
	return nemotale_event_is_cancel(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_long_press(struct nemoshow *show, void *event)
{
	return nemotale_event_is_long_press(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_touch_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_touch_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_touch_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_touch_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_touch_motion(struct nemoshow *show, void *event)
{
	return nemotale_event_is_touch_motion(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_touch_single_click(struct nemoshow *show, void *event)
{
	return nemotale_event_is_touch_single_click(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_enter(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_enter(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_leave(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_leave(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_button_down(struct nemoshow *show, void *event, uint32_t button)
{
	return nemotale_event_is_pointer_button_down(show->tale, (struct taleevent *)event, button);
}

static inline int nemoshow_event_is_pointer_button_up(struct nemoshow *show, void *event, uint32_t button)
{
	return nemotale_event_is_pointer_button_up(show->tale, (struct taleevent *)event, button);
}

static inline int nemoshow_event_is_pointer_motion(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_motion(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_axis(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_axis(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_pointer_single_click(struct nemoshow *show, void *event)
{
	return nemotale_event_is_pointer_single_click(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_enter(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_enter(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_leave(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_leave(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_up(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_keyboard_layout(struct nemoshow *show, void *event)
{
	return nemotale_event_is_keyboard_layout(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_enter(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_enter(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_leave(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_leave(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_translate(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_translate(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_rotate(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_rotate(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_button_down(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_button_down(show->tale, (struct taleevent *)event);
}

static inline int nemoshow_event_is_stick_button_up(struct nemoshow *show, void *event)
{
	return nemotale_event_is_stick_button_up(show->tale, (struct taleevent *)event);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
