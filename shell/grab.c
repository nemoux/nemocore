#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-server.h>

#include <grab.h>
#include <shell.h>
#include <canvas.h>
#include <actor.h>
#include <view.h>
#include <nemomisc.h>

static void shellgrab_handle_bin_destroy(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_destroy_listener);

	grab->bin = NULL;

	wl_list_remove(&grab->bin_destroy_listener.link);
	wl_list_remove(&grab->bin_ungrab_listener.link);
	wl_list_remove(&grab->bin_change_listener.link);
}

static void shellgrab_handle_bin_ungrab_pointer(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_ungrab_listener);

	nemoshell_end_pointer_shellgrab(grab);
	free(grab);
}

static void shellgrab_handle_bin_ungrab_touchpoint(struct wl_listener *listener, void *data)
{
	struct shellgrab *grab = (struct shellgrab *)container_of(listener, struct shellgrab, bin_ungrab_listener);

	nemoshell_miss_touchpoint_shellgrab(grab);
	free(grab);
}

static void actorgrab_handle_actor_destroy(struct wl_listener *listener, void *data)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(listener, struct actorgrab, actor_destroy_listener);

	grab->actor = NULL;
}

static void actorgrab_handle_actor_ungrab_pointer(struct wl_listener *listener, void *data)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(listener, struct actorgrab, actor_ungrab_listener);

	nemoshell_end_pointer_actorgrab(grab);
	free(grab);
}

static void actorgrab_handle_actor_ungrab_touchpoint(struct wl_listener *listener, void *data)
{
	struct actorgrab *grab = (struct actorgrab *)container_of(listener, struct actorgrab, actor_ungrab_listener);

	nemoshell_miss_touchpoint_actorgrab(grab);
	free(grab);
}

void nemoshell_start_pointer_shellgrab(struct shellgrab *grab, const struct nemopointer_grab_interface *interface, struct shellbin *bin, struct nemopointer *pointer)
{
	grab->base.pointer.interface = interface;
	grab->bin = bin;
	grab->bin_destroy_listener.notify = shellgrab_handle_bin_destroy;
	wl_signal_add(&bin->destroy_signal, &grab->bin_destroy_listener);
	grab->bin_ungrab_listener.notify = shellgrab_handle_bin_ungrab_pointer;
	wl_signal_add(&bin->ungrab_signal, &grab->bin_ungrab_listener);
	if (grab->bin->grabbed++ == 0)
		nemoview_set_state(bin->view, NEMOVIEW_GRAB_STATE);

	wl_list_init(&grab->bin_change_listener.link);

	nemopointer_start_grab(pointer, &grab->base.pointer);
}

void nemoshell_end_pointer_shellgrab(struct shellgrab *grab)
{
	if (grab->bin != NULL) {
		wl_list_remove(&grab->bin_destroy_listener.link);
		wl_list_remove(&grab->bin_ungrab_listener.link);
		wl_list_remove(&grab->bin_change_listener.link);
		if (--grab->bin->grabbed == 0)
			nemoview_put_state(grab->bin->view, NEMOVIEW_GRAB_STATE);
	}

	nemopointer_end_grab(grab->base.pointer.pointer);
}

void nemoshell_start_pointer_actorgrab(struct actorgrab *grab, const struct nemopointer_grab_interface *interface, struct nemoactor *actor, struct nemopointer *pointer)
{
	grab->base.pointer.interface = interface;
	grab->actor = actor;
	grab->actor_destroy_listener.notify = actorgrab_handle_actor_destroy;
	wl_signal_add(&actor->destroy_signal, &grab->actor_destroy_listener);
	grab->actor_ungrab_listener.notify = actorgrab_handle_actor_ungrab_pointer;
	wl_signal_add(&actor->ungrab_signal, &grab->actor_ungrab_listener);
	if (grab->actor->grabbed++ == 0)
		nemoview_set_state(actor->view, NEMOVIEW_GRAB_STATE);

	nemopointer_start_grab(pointer, &grab->base.pointer);
}

void nemoshell_end_pointer_actorgrab(struct actorgrab *grab)
{
	if (grab->actor != NULL) {
		wl_list_remove(&grab->actor_destroy_listener.link);
		wl_list_remove(&grab->actor_ungrab_listener.link);
		if (--grab->actor->grabbed == 0)
			nemoview_put_state(grab->actor->view, NEMOVIEW_GRAB_STATE);
	}

	nemopointer_end_grab(grab->base.pointer.pointer);
}

void nemoshell_start_touchpoint_shellgrab(struct shellgrab *grab, const struct touchpoint_grab_interface *interface, struct shellbin *bin, struct touchpoint *tp)
{
	grab->base.touchpoint.interface = interface;
	grab->bin = bin;
	grab->bin_destroy_listener.notify = shellgrab_handle_bin_destroy;
	wl_signal_add(&bin->destroy_signal, &grab->bin_destroy_listener);
	grab->bin_ungrab_listener.notify = shellgrab_handle_bin_ungrab_touchpoint;
	wl_signal_add(&bin->ungrab_signal, &grab->bin_ungrab_listener);
	if (grab->bin->grabbed++ == 0)
		nemoview_set_state(bin->view, NEMOVIEW_GRAB_STATE);

	wl_list_init(&grab->bin_change_listener.link);

	bin->canvas->touchid0 = 0;

	touchpoint_start_grab(tp, &grab->base.touchpoint);
}

void nemoshell_end_touchpoint_shellgrab(struct shellgrab *grab)
{
	if (grab->bin != NULL) {
		wl_list_remove(&grab->bin_destroy_listener.link);
		wl_list_remove(&grab->bin_ungrab_listener.link);
		wl_list_remove(&grab->bin_change_listener.link);
		if (--grab->bin->grabbed == 0)
			nemoview_put_state(grab->bin->view, NEMOVIEW_GRAB_STATE);
	}

	touchpoint_end_grab(grab->base.touchpoint.touchpoint);
}

void nemoshell_miss_touchpoint_shellgrab(struct shellgrab *grab)
{
	if (grab->bin != NULL) {
		wl_list_remove(&grab->bin_destroy_listener.link);
		wl_list_remove(&grab->bin_ungrab_listener.link);
		wl_list_remove(&grab->bin_change_listener.link);
		if (--grab->bin->grabbed == 0)
			nemoview_put_state(grab->bin->view, NEMOVIEW_GRAB_STATE);
	}

	touchpoint_end_grab(grab->base.touchpoint.touchpoint);
}

void nemoshell_start_touchpoint_actorgrab(struct actorgrab *grab, const struct touchpoint_grab_interface *interface, struct nemoactor *actor, struct touchpoint *tp)
{
	grab->base.touchpoint.interface = interface;
	grab->actor = actor;
	grab->actor_destroy_listener.notify = actorgrab_handle_actor_destroy;
	wl_signal_add(&actor->destroy_signal, &grab->actor_destroy_listener);
	grab->actor_ungrab_listener.notify = actorgrab_handle_actor_ungrab_touchpoint;
	wl_signal_add(&actor->ungrab_signal, &grab->actor_ungrab_listener);
	if (grab->actor->grabbed++ == 0)
		nemoview_set_state(actor->view, NEMOVIEW_GRAB_STATE);

	touchpoint_start_grab(tp, &grab->base.touchpoint);
}

void nemoshell_end_touchpoint_actorgrab(struct actorgrab *grab)
{
	if (grab->actor != NULL) {
		wl_list_remove(&grab->actor_destroy_listener.link);
		wl_list_remove(&grab->actor_ungrab_listener.link);
		if (--grab->actor->grabbed == 0)
			nemoview_put_state(grab->actor->view, NEMOVIEW_GRAB_STATE);
	}

	touchpoint_end_grab(grab->base.touchpoint.touchpoint);
}

void nemoshell_miss_touchpoint_actorgrab(struct actorgrab *grab)
{
	if (grab->actor != NULL) {
		wl_list_remove(&grab->actor_destroy_listener.link);
		wl_list_remove(&grab->actor_ungrab_listener.link);
		if (--grab->actor->grabbed == 0)
			nemoview_put_state(grab->actor->view, NEMOVIEW_GRAB_STATE);
	}

	touchpoint_end_grab(grab->base.touchpoint.touchpoint);
}
