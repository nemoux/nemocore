#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoegl.h>
#include <nemoshow.h>
#include <showhelper.h>
#include <nemomisc.h>

static void nemoshow_dispatch_canvas_resize(struct nemocanvas *canvas, int32_t width, int32_t height, int32_t fixed)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	if (width == 0 || height == 0)
		return;

	nemotool_resize_egl_canvas(scon->eglcanvas, width, height);

	nemoshow_set_size(show, width, height);

	nemoshow_render_one(show);

	nemotale_composite_egl_full(scon->tale);
}

static void nemoshow_dispatch_canvas_frame(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);
	struct nemoshow *show = (struct nemoshow *)nemotale_get_userdata(tale);

	if (secs == 0 && nsecs == 0) {
		nemocanvas_feedback(canvas);
	} else if (nemoshow_has_transition(show) != 0) {
		nemoshow_dispatch_transition(show, secs * 1000 + nsecs / 1000000);

		nemocanvas_feedback(canvas);
	}

	nemoshow_destroy_transition(show);

	nemoshow_render_one(show);

	nemotale_composite_egl(tale, NULL);
}

static int nemoshow_dispatch_canvas_event(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	struct nemotale *tale = (struct nemotale *)nemocanvas_get_userdata(canvas);

	if (type & NEMOTOOL_POINTER_ENTER_EVENT) {
		nemotale_push_pointer_enter_event(tale, event->serial, event->device, event->x, event->y);
	} else if (type & NEMOTOOL_POINTER_LEAVE_EVENT) {
		nemotale_push_pointer_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_POINTER_MOTION_EVENT) {
		nemotale_push_pointer_motion_event(tale, event->serial, event->device, event->time, event->x, event->y);
	} else if (type & NEMOTOOL_POINTER_BUTTON_EVENT) {
		if (event->state == WL_POINTER_BUTTON_STATE_PRESSED)
			nemotale_push_pointer_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_pointer_up_event(tale, event->serial, event->device, event->time, event->value);
	} else if (type & NEMOTOOL_POINTER_AXIS_EVENT) {
	} else if (type & NEMOTOOL_KEYBOARD_ENTER_EVENT) {
		nemotale_push_keyboard_enter_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_KEYBOARD_LEAVE_EVENT) {
		nemotale_push_keyboard_leave_event(tale, event->serial, event->device);
	} else if (type & NEMOTOOL_KEYBOARD_KEY_EVENT) {
		if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED)
			nemotale_push_keyboard_down_event(tale, event->serial, event->device, event->time, event->value);
		else
			nemotale_push_keyboard_up_event(tale, event->serial, event->device, event->time, event->value);
	} else if (type & NEMOTOOL_KEYBOARD_MODIFIERS_EVENT) {
	} else if (type & NEMOTOOL_TOUCH_DOWN_EVENT) {
		nemotale_push_touch_down_event(tale, event->serial, event->device, event->time, event->x, event->y, event->gx, event->gy);
	} else if (type & NEMOTOOL_TOUCH_UP_EVENT) {
		nemotale_push_touch_up_event(tale, event->serial, event->device, event->time);
	} else if (type & NEMOTOOL_TOUCH_MOTION_EVENT) {
		nemotale_push_touch_motion_event(tale, event->serial, event->device, event->time, event->x, event->y, event->gx, event->gy);
	}

	return 0;
}

static void nemoshow_dispatch_timer(struct nemotimer *timer, void *data)
{
	struct showcontext *scon = (struct showcontext *)data;

	nemotimer_set_timeout(timer, 500);

	nemotale_push_timer_event(scon->tale, time_current_msecs());
}

struct nemoshow *nemoshow_create_on_tale(struct nemotool *tool, int32_t width, int32_t height, nemotale_dispatch_event_t dispatch)
{
	struct showcontext *scon;
	struct nemoshow *show;
	struct nemotimer *timer;

	scon = (struct showcontext *)malloc(sizeof(struct showcontext));
	if (scon == NULL)
		return NULL;
	memset(scon, 0, sizeof(struct showcontext));

	scon->timer = timer = nemotimer_create(tool);
	if (timer == NULL)
		goto err1;
	nemotimer_set_callback(timer, nemoshow_dispatch_timer);
	nemotimer_set_timeout(timer, 500);
	nemotimer_set_userdata(timer, scon);

	scon->tool = tool;
	scon->width = width;
	scon->height = height;

	scon->egl = nemotool_create_egl(scon->tool);

	scon->eglcanvas = nemotool_create_egl_canvas(scon->egl, width, height);
	scon->canvas = NTEGL_CANVAS(scon->eglcanvas);

	nemocanvas_set_nemosurface(scon->canvas, NEMO_SHELL_SURFACE_TYPE_NORMAL);
	nemocanvas_set_dispatch_resize(scon->canvas, nemoshow_dispatch_canvas_resize);
	nemocanvas_set_dispatch_frame(scon->canvas, nemoshow_dispatch_canvas_frame);
	nemocanvas_set_dispatch_event(scon->canvas, nemoshow_dispatch_canvas_event);

	scon->tale = nemotale_create_gl();
	nemotale_set_backend(scon->tale,
			nemotale_create_egl(
				NTEGL_DISPLAY(scon->egl),
				NTEGL_CONTEXT(scon->egl),
				NTEGL_CONFIG(scon->egl),
				(EGLNativeWindowType)NTEGL_WINDOW(scon->eglcanvas)));
	nemotale_set_dispatch_event(scon->tale, dispatch);

	nemoshow_initialize();

	show = nemoshow_create();
	nemoshow_set_tale(show, scon->tale);
	nemoshow_set_context(show, scon);

	nemocanvas_set_userdata(NTEGL_CANVAS(scon->eglcanvas), scon->tale);
	nemotale_set_userdata(scon->tale, show);

	return show;

err1:
	free(scon);

	return NULL;
}

void nemoshow_destroy_on_tale(struct nemoshow *show)
{
	struct showcontext *scon = (struct showcontext *)nemoshow_get_context(show);

	nemoshow_destroy(show);

	nemoshow_finalize();

	nemotale_destroy_gl(scon->tale);

	nemotool_destroy_egl_canvas(scon->eglcanvas);
	nemotool_destroy_egl(scon->egl);

	free(scon);
}
