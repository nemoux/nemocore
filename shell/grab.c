#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <grab.h>
#include <shell.h>
#include <compz.h>
#include <canvas.h>
#include <view.h>
#include <nemomisc.h>

void nemoshell_miss_shellgrab(struct shellgrab *grab)
{
	wl_list_remove(&grab->bin_destroy_listener.link);
	wl_list_remove(&grab->bin_ungrab_listener.link);
	wl_list_remove(&grab->bin_change_listener.link);

	nemoview_ungrab(grab->bin->view);

	grab->bin = NULL;
}

static void shellgrab_handle_bin_destroy(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_destroy_listener);

	nemoshell_miss_shellgrab(grab);
}

static void shellgrab_handle_bin_ungrab_pointer(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_ungrab_listener);

	nemopointer_cancel_grab(grab->base.pointer.pointer);
}

static void shellgrab_handle_bin_ungrab_touchpoint(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_ungrab_listener);

	touchpoint_cancel_grab(grab->base.touchpoint.touchpoint);
}

void nemoshell_start_pointer_shellgrab(struct nemoshell *shell, struct shellgrab *grab, const struct nemopointer_grab_interface *interface, struct shellbin *bin, struct nemopointer *pointer)
{
	grab->base.pointer.interface = interface;
	grab->bin = bin;
	grab->bin_destroy_listener.notify = shellgrab_handle_bin_destroy;
	wl_signal_add(&bin->destroy_signal, &grab->bin_destroy_listener);
	grab->bin_ungrab_listener.notify = shellgrab_handle_bin_ungrab_pointer;
	wl_signal_add(&bin->ungrab_signal, &grab->bin_ungrab_listener);

	nemoview_grab(bin->view);

	wl_list_init(&grab->bin_change_listener.link);

	nemopointer_start_grab(pointer, &grab->base.pointer);
}

void nemoshell_end_pointer_shellgrab(struct shellgrab *grab)
{
	if (grab->bin != NULL)
		nemoshell_miss_shellgrab(grab);

	nemopointer_end_grab(grab->base.pointer.pointer);
}

void nemoshell_start_touchpoint_shellgrab(struct nemoshell *shell, struct shellgrab *grab, const struct touchpoint_grab_interface *interface, struct shellbin *bin, struct touchpoint *tp)
{
	grab->base.touchpoint.interface = interface;
	grab->bin = bin;
	grab->bin_destroy_listener.notify = shellgrab_handle_bin_destroy;
	wl_signal_add(&bin->destroy_signal, &grab->bin_destroy_listener);
	grab->bin_ungrab_listener.notify = shellgrab_handle_bin_ungrab_touchpoint;
	wl_signal_add(&bin->ungrab_signal, &grab->bin_ungrab_listener);

	nemoview_grab(bin->view);

	wl_list_init(&grab->bin_change_listener.link);

	touchpoint_start_grab(tp, &grab->base.touchpoint);
}

void nemoshell_end_touchpoint_shellgrab(struct shellgrab *grab)
{
	if (grab->bin != NULL)
		nemoshell_miss_shellgrab(grab);

	touchpoint_end_grab(grab->base.touchpoint.touchpoint);
}

static int nemoshell_dispatch_touchgrab_timeout(void *data)
{
	struct touchgrab *grab = (struct touchgrab *)data;
	struct touchpoint *tp = grab->tp;
	uint32_t msecs = time_current_msecs();

	grab->samples[grab->esample].x = tp->x;
	grab->samples[grab->esample].y = tp->y;
	grab->samples[grab->esample].time = msecs;

	grab->nsamples++;

	grab->esample = (grab->esample + 1) % NEMOSHELL_TOUCH_SAMPLE_MAX;

	if (grab->ssample == grab->esample)
		grab->ssample = (grab->ssample + 1) % NEMOSHELL_TOUCH_SAMPLE_MAX;

	wl_event_source_timer_update(grab->timer, grab->timeout);

	return 1;
}

void nemoshell_start_touchgrab(struct nemoshell *shell, struct touchgrab *grab, struct touchpoint *tp, uint32_t timeout)
{
	grab->tp = tp;

	grab->timer = wl_event_loop_add_timer(shell->compz->loop, nemoshell_dispatch_touchgrab_timeout, grab);
	grab->timeout = timeout;

	wl_event_source_timer_update(grab->timer, grab->timeout);
}

void nemoshell_end_touchgrab(struct touchgrab *grab)
{
	wl_event_source_remove(grab->timer);
}

void nemoshell_update_touchgrab_velocity(struct touchgrab *grab, uint32_t nsamples, float *dx, float *dy)
{
	float x0, y0;
	float x1, y1;
	uint32_t t0, t1;
	uint32_t i0, i1;

	if (grab->nsamples < nsamples) {
		*dx = 0.0f;
		*dy = 0.0f;
	} else {
		i0 = (grab->esample - nsamples) % NEMOSHELL_TOUCH_SAMPLE_MAX;
		i1 = (grab->esample - 1) % NEMOSHELL_TOUCH_SAMPLE_MAX;

		x0 = grab->samples[i0].x;
		y0 = grab->samples[i0].y;
		t0 = grab->samples[i0].time;

		x1 = grab->samples[i1].x;
		y1 = grab->samples[i1].y;
		t1 = grab->samples[i1].time;

		*dx = (x1 - x0) / (float)(t1 - t0);
		*dy = (y1 - y0) / (float)(t1 - t0);
	}
}

int nemoshell_check_touchgrab_duration(struct touchgrab *grab, uint32_t nsamples, uint32_t max_duration)
{
	uint32_t t0, t1;
	uint32_t i0, i1;

	if (grab->nsamples < nsamples)
		return 0;

	i0 = (grab->esample - nsamples) % NEMOSHELL_TOUCH_SAMPLE_MAX;
	i1 = (grab->esample - 1) % NEMOSHELL_TOUCH_SAMPLE_MAX;

	t0 = grab->samples[i0].time;
	t1 = grab->samples[i1].time;

	if (t1 - t0 > max_duration)
		return 0;

	return 1;
}

int nemoshell_check_touchgrab_velocity(struct touchgrab *grab, uint32_t nsamples, double min_velocity)
{
	float x0, y0;
	float x1, y1;
	float dx, dy;
	uint32_t t0, t1;
	uint32_t i0, i1;

	if (grab->nsamples < nsamples)
		return 0;

	i0 = (grab->esample - nsamples) % NEMOSHELL_TOUCH_SAMPLE_MAX;
	i1 = (grab->esample - 1) % NEMOSHELL_TOUCH_SAMPLE_MAX;

	x0 = grab->samples[i0].x;
	y0 = grab->samples[i0].y;
	t0 = grab->samples[i0].time;

	x1 = grab->samples[i1].x;
	y1 = grab->samples[i1].y;
	t1 = grab->samples[i1].time;

	dx = (x1 - x0) / (float)(t1 - t0);
	dy = (y1 - y0) / (float)(t1 - t0);

	if (sqrtf(dx * dx + dy * dy) < min_velocity)
		return 0;

	return 1;
}
