#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/epoll.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>

#include <pixman.h>
#include <cairo.h>

#include <wayland-client.h>
#include <wayland-nemo-seat-client-protocol.h>
#include <wayland-nemo-sound-client-protocol.h>
#include <wayland-nemo-shell-client-protocol.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemooutput.h>
#include <nemomisc.h>
#include <oshelper.h>

static void pointer_handle_enter(void *data, struct nemo_pointer *pointer, uint32_t serial, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;
			event.x = wl_fixed_to_double(sx);
			event.y = wl_fixed_to_double(sy);

			canvas->dispatch_event(canvas, NEMOTOOL_POINTER_ENTER_EVENT, &event);
		}
	}
}

static void pointer_handle_leave(void *data, struct nemo_pointer *pointer, uint32_t serial, struct wl_surface *surface, int32_t id)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;

			canvas->dispatch_event(canvas, NEMOTOOL_POINTER_LEAVE_EVENT, &event);
		}
	}
}

static void pointer_handle_motion(void *data, struct nemo_pointer *pointer, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.time = time;
			event.x = wl_fixed_to_double(sx);
			event.y = wl_fixed_to_double(sy);

			canvas->dispatch_event(canvas, NEMOTOOL_POINTER_MOTION_EVENT, &event);
		}
	}
}

static void pointer_handle_button(void *data, struct nemo_pointer *pointer, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, uint32_t button, uint32_t state)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;
			event.time = time;
			event.value = button;
			event.state = state;

			canvas->dispatch_event(canvas, NEMOTOOL_POINTER_BUTTON_EVENT, &event);
		}
	}
}

static void pointer_handle_axis(void *data, struct nemo_pointer *pointer, uint32_t time, struct wl_surface *surface, int32_t id, uint32_t axis, wl_fixed_t value)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.time = time;
			event.value = wl_fixed_to_double(value);
			event.state = axis;

			canvas->dispatch_event(canvas, NEMOTOOL_POINTER_AXIS_EVENT, &event);
		}
	}
}

static const struct nemo_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis
};

static void keyboard_handle_keymap(void *data, struct nemo_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
	struct nemotool *tool = (struct nemotool *)data;
	struct xkb_keymap *keymap;
	struct xkb_state *state;
	char *mapstr;

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		return;
	}

	mapstr = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	if (mapstr == MAP_FAILED) {
		close(fd);
		return;
	}

	keymap = xkb_map_new_from_string(tool->xkb.context, mapstr, XKB_KEYMAP_FORMAT_TEXT_V1, 0);
	munmap(mapstr, size);
	close(fd);

	if (keymap == NULL)
		return;

	state = xkb_state_new(keymap);
	if (state == NULL) {
		xkb_map_unref(keymap);
		return;
	}

	xkb_keymap_unref(tool->xkb.keymap);
	xkb_state_unref(tool->xkb.state);

	tool->xkb.keymap = keymap;
	tool->xkb.state = state;

	tool->xkb.control_mask =
		1 << xkb_map_mod_get_index(tool->xkb.keymap, "Control");
	tool->xkb.alt_mask =
		1 << xkb_map_mod_get_index(tool->xkb.keymap, "Mod1");
	tool->xkb.shift_mask =
		1 << xkb_map_mod_get_index(tool->xkb.keymap, "Shift");
}

static void keyboard_handle_enter(void *data, struct nemo_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, int32_t id, struct wl_array *keys)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;

			canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_ENTER_EVENT, &event);
		}
	}
}

static void keyboard_handle_leave(void *data, struct nemo_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, int32_t id)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;

			canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_LEAVE_EVENT, &event);
		}
	}
}

static void keyboard_handle_key(void *data, struct nemo_keyboard *keyboard, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, uint32_t key, uint32_t state)
{
	if (surface != NULL) {
		struct nemotool *tool = (struct nemotool *)data;
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;
			event.time = time;
			event.value = key;
			event.state = state;

			canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_KEY_EVENT, &event);
		}
	}
}

static void keyboard_handle_modifiers(void *data, struct nemo_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, int32_t id, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	if (surface != NULL) {
		struct nemotool *tool = (struct nemotool *)data;
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		xkb_mod_mask_t mask;

		if (tool->xkb.keymap == NULL)
			return;

		xkb_state_update_mask(tool->xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
		mask = xkb_state_serialize_mods(tool->xkb.state, XKB_STATE_DEPRESSED | XKB_STATE_LATCHED);
		tool->modifiers = 0;
		if (mask & tool->xkb.control_mask)
			tool->modifiers |= NEMO_MOD_CONTROL_MASK;
		if (mask & tool->xkb.alt_mask)
			tool->modifiers |= NEMO_MOD_ALT_MASK;
		if (mask & tool->xkb.shift_mask)
			tool->modifiers |= NEMO_MOD_SHIFT_MASK;

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;

			canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_MODIFIERS_EVENT, &event);
		}
	}
}

static const struct nemo_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers
};

static void touch_handle_down(void *data, struct nemo_touch *touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;
			event.time = time;
			event.x = wl_fixed_to_double(sx);
			event.y = wl_fixed_to_double(sy);

			canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_DOWN_EVENT, &event);
		}
	}
}

static void touch_handle_up(void *data, struct nemo_touch *touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t dx, wl_fixed_t dy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.serial = serial;
			event.time = time;
			event.x = wl_fixed_to_double(dx);
			event.y = wl_fixed_to_double(dy);

			canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_UP_EVENT, &event);
		}
	}
}

static void touch_handle_motion(void *data, struct nemo_touch *touch, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas != NULL && canvas->dispatch_event != NULL) {
			struct nemoevent event;

			event.device = id;
			event.time = time;
			event.x = wl_fixed_to_double(sx);
			event.y = wl_fixed_to_double(sy);

			canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_MOTION_EVENT, &event);
		}
	}
}

static void touch_handle_frame(void *data, struct nemo_touch *touch)
{
}

static void touch_handle_cancel(void *data, struct nemo_touch *touch)
{
}

static const struct nemo_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel
};

static void seat_handle_capabilities(void *data, struct nemo_seat *seat, enum nemo_seat_capability caps)
{
	struct nemotool *tool = (struct nemotool *)data;

	if ((caps & NEMO_SEAT_CAPABILITY_POINTER) && tool->pointer == NULL) {
		tool->pointer = nemo_seat_get_pointer(seat);
		nemo_pointer_add_listener(tool->pointer, &pointer_listener, data);
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && tool->pointer != NULL) {
		nemo_pointer_destroy(tool->pointer);
		tool->pointer = NULL;
	}

	if ((caps & NEMO_SEAT_CAPABILITY_KEYBOARD) && tool->keyboard == NULL) {
		tool->keyboard = nemo_seat_get_keyboard(seat);
		nemo_keyboard_add_listener(tool->keyboard, &keyboard_listener, data);
	} else if(!(caps & NEMO_SEAT_CAPABILITY_KEYBOARD) && tool->keyboard != NULL) {
		nemo_keyboard_destroy(tool->keyboard);
		tool->keyboard = NULL;
	}

	if ((caps & NEMO_SEAT_CAPABILITY_TOUCH) && tool->touch == NULL) {
		tool->touch = nemo_seat_get_touch(seat);
		nemo_touch_add_listener(tool->touch, &touch_listener, data);
	} else if (!(caps & NEMO_SEAT_CAPABILITY_TOUCH) && tool->touch != NULL) {
		nemo_touch_destroy(tool->touch);
		tool->touch = NULL;
	}
}

static void seat_handle_name(void *data, struct nemo_seat *seat, const char *name)
{
}

static const struct nemo_seat_listener seat_listener = {
	seat_handle_capabilities,
	seat_handle_name
};

static void sound_handle_enter(void *data, struct nemo_sound *sound, struct wl_surface *surface, const char *device)
{
	struct nemotool *tool = (struct nemotool *)data;

	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas->dispatch_sound != NULL) {
			canvas->dispatch_sound(canvas, device, 0);
		}
	}
}

static void sound_handle_leave(void *data, struct nemo_sound *sound, struct wl_surface *surface, const char *device)
{
	struct nemotool *tool = (struct nemotool *)data;

	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);

		if (canvas->dispatch_sound != NULL) {
			canvas->dispatch_sound(canvas, device, 1);
		}
	}
}

static const struct nemo_sound_listener sound_listener = {
	sound_handle_enter,
	sound_handle_leave
};

static void shm_format(void *data, struct wl_shm *shm, uint32_t format)
{
	struct nemotool *tool = (struct nemotool *)data;

	tool->formats |= (1 << format);
}

static struct wl_shm_listener shm_listener = {
	shm_format
};

static void nemo_shell_ping(void *data, struct nemo_shell *shell, uint32_t serial)
{
	struct nemotool *tool = (struct nemotool *)data;

	nemo_shell_pong(tool->shell, serial);
}

static const struct nemo_shell_listener nemo_shell_listener = {
	nemo_shell_ping
};

static void presentation_clock_id(void *data, struct presentation *presentation, uint32_t clock_id)
{
	struct nemotool *tool = (struct nemotool *)data;

	tool->clock_id = clock_id;
}

static const struct presentation_listener presentation_listener = {
	presentation_clock_id
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct nemotool *tool = (struct nemotool *)data;
	struct nemoglobal *global;

	global = (struct nemoglobal *)malloc(sizeof(struct nemoglobal));
	if (global == NULL)
		return;
	global->name = id;
	global->version = version;
	global->interface = strdup(interface);

	nemolist_insert(&tool->global_list, &global->link);

	if (strcmp(interface, "wl_compositor") == 0) {
		tool->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "wl_subcompositor") == 0) {
		tool->subcompositor = wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
	} else if (strcmp(interface, "wl_output") == 0) {
		nemooutput_register(tool, id);
	} else if (strcmp(interface, "nemo_seat") == 0) {
		tool->seat = wl_registry_bind(registry, id, &nemo_seat_interface, 1);
		nemo_seat_add_listener(tool->seat, &seat_listener, data);
	} else if (strcmp(interface, "nemo_sound") == 0) {
		tool->sound = wl_registry_bind(registry, id, &nemo_sound_interface, 1);
		nemo_sound_add_listener(tool->sound, &sound_listener, data);
	} else if (strcmp(interface, "wl_shm") == 0) {
		tool->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
		wl_shm_add_listener(tool->shm, &shm_listener, data);
	} else if (strcmp(interface, "nemo_shell") == 0) {
		tool->shell = wl_registry_bind(registry, id, &nemo_shell_interface, 1);
		nemo_shell_use_unstable_version(tool->shell, 1);
		nemo_shell_add_listener(tool->shell, &nemo_shell_listener, data);
	} else if (strcmp(interface, "presentation") == 0) {
		tool->presentation = wl_registry_bind(registry, id, &presentation_interface, 1);
		presentation_add_listener(tool->presentation, &presentation_listener, data);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	struct nemotool *tool = (struct nemotool *)data;
	struct nemoglobal *global, *next;

	nemolist_for_each_safe(global, next, &tool->global_list, link) {
		if (global->name == name) {
			if (strcmp(global->interface, "wl_output") == 0)
				nemooutput_unregister(tool, name);

			nemolist_remove(&global->link);

			free(global->interface);
			free(global);
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static void nemotool_dispatch_wayland_display(struct nemotask *task, uint32_t events)
{
	struct nemotool *tool = (struct nemotool *)container_of(task, struct nemotool, display_task);
	struct epoll_event ep;
	int ret;

	tool->display_events = events;

	if (events & EPOLLERR || events & EPOLLHUP) {
		tool->running = 0;
		return;
	}

	if (events & EPOLLIN) {
		ret = wl_display_dispatch(tool->display);
		if (ret == -1) {
			tool->running = 0;
			return;
		}
	}

	if (events & EPOLLOUT) {
		ret = wl_display_flush(tool->display);
		if (ret == 0) {
			ep.events = EPOLLIN | EPOLLERR | EPOLLHUP;
			ep.data.ptr = &tool->display_task;
			epoll_ctl(tool->epoll_fd, EPOLL_CTL_MOD, tool->display_fd, &ep);
		} else if (ret == -1 && errno != EAGAIN) {
			tool->running = 0;
			return;
		}
	}
}

void nemotool_connect_wayland(struct nemotool *tool, const char *name)
{
	tool->display = wl_display_connect(name);

	tool->display_fd = wl_display_get_fd(tool->display);
	tool->display_task.dispatch = nemotool_dispatch_wayland_display;

	nemotool_watch_fd(tool, tool->display_fd, EPOLLIN | EPOLLERR | EPOLLHUP, &tool->display_task);

	tool->registry = wl_display_get_registry(tool->display);
	wl_registry_add_listener(tool->registry, &registry_listener, tool);

	wl_display_dispatch(tool->display);
}

void nemotool_disconnect_wayland(struct nemotool *tool)
{
	wl_display_disconnect(tool->display);

	tool->display = NULL;
}

void nemotool_dispatch(struct nemotool *tool)
{
	struct nemotask *task;
	struct epoll_event ep[16];
	int i, count, ret;

	while (!nemolist_empty(&tool->idle_list)) {
		task = (struct nemotask *)container_of(tool->idle_list.prev, struct nemotask, link);
		nemolist_remove(&task->link);
		nemolist_init(&task->link);
		task->dispatch(task, 0);
	}

	if (tool->display != NULL) {
		wl_display_dispatch_pending(tool->display);

		ret = wl_display_flush(tool->display);
		if (ret < 0 && errno == EAGAIN) {
			ep[0].events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
			ep[0].data.ptr = &tool->display_task;

			epoll_ctl(tool->epoll_fd, EPOLL_CTL_MOD, tool->display_fd, &ep[0]);
		} else if (ret < 0) {
			return;
		}
	}

	count = epoll_wait(tool->epoll_fd, ep, ARRAY_LENGTH(ep), -1);
	for (i = 0; i < count; i++) {
		task = ep[i].data.ptr;
		task->dispatch(task, ep[i].events);
	}
}

void nemotool_run(struct nemotool *tool)
{
	while (1) {
		if (tool->running == 0)
			break;

		nemotool_dispatch(tool);
	}
}

void nemotool_flush(struct nemotool *tool)
{
	wl_display_flush(tool->display);
}

void nemotool_exit(struct nemotool *tool)
{
	tool->running = 0;
}

void nemotool_watch_fd(struct nemotool *tool, int fd, uint32_t events, struct nemotask *task)
{
	struct epoll_event ep;

	ep.events = events;
	ep.data.ptr = task;
	epoll_ctl(tool->epoll_fd, EPOLL_CTL_ADD, fd, &ep);
}

void nemotool_unwatch_fd(struct nemotool *tool, int fd)
{
	epoll_ctl(tool->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

int nemotool_get_fd(struct nemotool *tool)
{
	return tool->epoll_fd;
}

struct nemotool *nemotool_create(void)
{
	struct nemotool *tool;

	tool = (struct nemotool *)malloc(sizeof(struct nemotool));
	if (tool == NULL)
		return NULL;
	memset(tool, 0, sizeof(struct nemotool));

	tool->epoll_fd = os_epoll_create_cloexec();
	if (tool->epoll_fd < 0)
		goto err1;

	tool->xkb.context = xkb_context_new(0);
	if (tool->xkb.context == NULL)
		goto err2;

	nemolist_init(&tool->global_list);
	nemolist_init(&tool->output_list);

	nemolist_init(&tool->idle_list);

	tool->running = 1;

	return tool;

err2:
	close(tool->epoll_fd);

err1:
	free(tool);

	return NULL;
}

void nemotool_destroy(struct nemotool *tool)
{
	struct nemooutput *output, *noutput;
	struct nemoglobal *global, *nglobal;

	nemolist_for_each_safe(output, noutput, &tool->output_list, link) {
		nemooutput_destroy(output);
	}

	nemolist_for_each_safe(global, nglobal, &tool->global_list, link) {
		nemolist_remove(&global->link);

		free(global->interface);
		free(global);
	}

	xkb_context_unref(tool->xkb.context);

	close(tool->epoll_fd);

	free(tool);
}

void nemotool_idle_task(struct nemotool *tool, struct nemotask *task)
{
	nemolist_insert(&tool->idle_list, &task->link);
}

void nemotool_remove_task(struct nemotool *tool, struct nemotask *task)
{
	nemolist_remove(&task->link);
	nemolist_init(&task->link);
}

struct nemoqueue *nemotool_create_queue(struct nemotool *tool)
{
	struct nemoqueue *queue;

	queue = (struct nemoqueue *)malloc(sizeof(struct nemoqueue));
	if (queue == NULL)
		return NULL;
	memset(queue, 0, sizeof(struct nemoqueue));

	queue->tool = tool;

	queue->queue = wl_display_create_queue(tool->display);
	if (queue->queue == NULL)
		goto err1;

	return queue;

err1:
	free(queue);

	return NULL;
}

void nemotool_destroy_queue(struct nemoqueue *queue)
{
	wl_event_queue_destroy(queue->queue);

	free(queue);
}

int nemotool_dispatch_queue(struct nemoqueue *queue)
{
	return wl_display_dispatch_queue(queue->tool->display, queue->queue);
}

uint32_t nemotool_get_keysym(struct nemotool *tool, uint32_t code)
{
	if (tool->xkb.state != NULL) {
		const xkb_keysym_t *syms;
		int numsyms;

		numsyms = xkb_key_get_syms(tool->xkb.state, code + 8, &syms);
		if (numsyms == 1) {
			return syms[0] & 0xff;
		}
	}

	return 0;
}
