#ifndef	__NEMO_SHELL_GRAB_H__
#define	__NEMO_SHELL_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <shell.h>
#include <pointer.h>
#include <keyboard.h>
#include <touch.h>

#define NEMOSHELL_TOUCH_SAMPLE_MAX				(64)
#define NEMOSHELL_TOUCH_DEFAULT_TIMEOUT		(1000.0f / 120.0f)

struct shellbin;

struct shellgrab {
	union {
		struct nemopointer_grab pointer;
		struct nemokeyboard_grab keyboard;
		struct touchpoint_grab touchpoint;
	} base;

	struct shellbin *bin;
	struct wl_listener bin_destroy_listener;
	struct wl_listener bin_ungrab_listener;
	struct wl_listener bin_change_listener;
};

struct touchsample {
	float x, y;

	uint32_t time;
};

struct touchgrab {
	struct touchpoint *tp;

	struct touchsample samples[NEMOSHELL_TOUCH_SAMPLE_MAX];
	uint32_t nsamples;
	uint32_t ssample, esample;

	struct wl_event_source *timer;
	uint32_t timeout;
};

extern void nemoshell_start_pointer_shellgrab(struct nemoshell *shell, struct shellgrab *grab, const struct nemopointer_grab_interface *interface, struct shellbin *bin, struct nemopointer *pointer);
extern void nemoshell_end_pointer_shellgrab(struct shellgrab *grab);

extern void nemoshell_start_touchpoint_shellgrab(struct nemoshell *shell, struct shellgrab *grab, const struct touchpoint_grab_interface *interface, struct shellbin *bin, struct touchpoint *tp);
extern void nemoshell_end_touchpoint_shellgrab(struct shellgrab *grab);

extern void nemoshell_miss_shellgrab(struct shellgrab *grab);

extern void nemoshell_start_touchgrab(struct nemoshell *shell, struct touchgrab *grab, struct touchpoint *tp, uint32_t timeout);
extern void nemoshell_end_touchgrab(struct touchgrab *grab);

extern void nemoshell_update_touchgrab_velocity(struct touchgrab *grab, uint32_t nsamples, float *dx, float *dy);
extern int nemoshell_check_touchgrab_duration(struct touchgrab *grab, uint32_t nsamples, uint32_t max_duration);
extern int nemoshell_check_touchgrab_velocity(struct touchgrab *grab, uint32_t nsamples, double min_velocity);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
