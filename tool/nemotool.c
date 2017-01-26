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

#include <wayland-client.h>
#include <wayland-presentation-timing-client-protocol.h>
#include <wayland-nemo-seat-client-protocol.h>
#include <wayland-nemo-sound-client-protocol.h>
#include <wayland-nemo-shell-client-protocol.h>
#include <wayland-nemo-client-client-protocol.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemooutput.h>
#include <nemoegl.h>
#include <nemomisc.h>

static void pointer_handle_enter(void *data, struct nemo_pointer *pointer, uint32_t serial, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;
		event.x = wl_fixed_to_double(sx);
		event.y = wl_fixed_to_double(sy);

		canvas->dispatch_event(canvas, NEMOTOOL_POINTER_ENTER_EVENT, &event);
	}
}

static void pointer_handle_leave(void *data, struct nemo_pointer *pointer, uint32_t serial, struct wl_surface *surface, int32_t id)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;

		canvas->dispatch_event(canvas, NEMOTOOL_POINTER_LEAVE_EVENT, &event);
	}
}

static void pointer_handle_motion(void *data, struct nemo_pointer *pointer, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.time = time;
		event.x = wl_fixed_to_double(sx);
		event.y = wl_fixed_to_double(sy);

		canvas->dispatch_event(canvas, NEMOTOOL_POINTER_MOTION_EVENT, &event);
	}
}

static void pointer_handle_button(void *data, struct nemo_pointer *pointer, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, uint32_t button, uint32_t state)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;
		event.time = time;
		event.value = button;
		event.state = state;

		canvas->dispatch_event(canvas, NEMOTOOL_POINTER_BUTTON_EVENT, &event);
	}
}

static void pointer_handle_axis(void *data, struct nemo_pointer *pointer, uint32_t time, struct wl_surface *surface, int32_t id, uint32_t axis, wl_fixed_t value)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.time = time;
		event.r = wl_fixed_to_double(value);
		event.state = axis;

		canvas->dispatch_event(canvas, NEMOTOOL_POINTER_AXIS_EVENT, &event);
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

	if (tool->xkb.keymap != NULL)
		xkb_keymap_unref(tool->xkb.keymap);
	if (tool->xkb.state != NULL)
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
		struct nemoevent event;

		event.device = id;
		event.serial = serial;

		canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_ENTER_EVENT, &event);
	}
}

static void keyboard_handle_leave(void *data, struct nemo_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, int32_t id)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;

		canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_LEAVE_EVENT, &event);
	}
}

static void keyboard_handle_key(void *data, struct nemo_keyboard *keyboard, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, uint32_t key, uint32_t state)
{
	if (surface != NULL) {
		struct nemotool *tool = (struct nemotool *)data;
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;
		event.time = time;
		event.value = key;
		event.state = state;

		canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_KEY_EVENT, &event);
	}
}

static void keyboard_handle_modifiers(void *data, struct nemo_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, int32_t id, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	if (surface != NULL) {
		struct nemotool *tool = (struct nemotool *)data;
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;
		xkb_mod_mask_t mask;

		if (tool->xkb.keymap == NULL)
			return;

		xkb_state_update_mask(tool->xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
		mask = xkb_state_serialize_mods(tool->xkb.state, XKB_STATE_DEPRESSED | XKB_STATE_LATCHED);
		tool->modifiers = 0;
		if (mask & tool->xkb.control_mask)
			tool->modifiers |= NEMOMOD_CONTROL_MASK;
		if (mask & tool->xkb.alt_mask)
			tool->modifiers |= NEMOMOD_ALT_MASK;
		if (mask & tool->xkb.shift_mask)
			tool->modifiers |= NEMOMOD_SHIFT_MASK;

		event.device = id;
		event.serial = serial;

		canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_MODIFIERS_EVENT, &event);
	}
}

static void keyboard_handle_layout(void *data, struct nemo_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, int32_t id, const char *name)
{
	if (surface != NULL) {
		struct nemotool *tool = (struct nemotool *)data;
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;
		event.name = name;

		canvas->dispatch_event(canvas, NEMOTOOL_KEYBOARD_LAYOUT_EVENT, &event);
	}
}

static const struct nemo_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers,
	keyboard_handle_layout
};

void nemotool_keyboard_enter(struct nemotool *tool)
{
	nemo_keyboard_enter(tool->keyboard);
}

void nemotool_keyboard_leave(struct nemotool *tool)
{
	nemo_keyboard_leave(tool->keyboard);
}

void nemotool_keyboard_key(struct nemotool *tool, uint32_t time, uint32_t key, int pressed)
{
	nemo_keyboard_key(tool->keyboard, time, key, pressed != 0 ? NEMO_KEYBOARD_KEY_STATE_PRESSED : NEMO_KEYBOARD_KEY_STATE_RELEASED);
}

void nemotool_keyboard_layout(struct nemotool *tool, const char *name)
{
	nemo_keyboard_layout(tool->keyboard, name);
}

static void touch_handle_down(void *data, struct nemo_touch *touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t x, wl_fixed_t y)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;
		event.time = time;
		event.x = wl_fixed_to_double(sx);
		event.y = wl_fixed_to_double(sy);
		event.gx = wl_fixed_to_double(x);
		event.gy = wl_fixed_to_double(y);

		canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_DOWN_EVENT, &event);
	}
}

static void touch_handle_up(void *data, struct nemo_touch *touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.serial = serial;
		event.time = time;

		canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_UP_EVENT, &event);
	}
}

static void touch_handle_motion(void *data, struct nemo_touch *touch, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t sx, wl_fixed_t sy, wl_fixed_t x, wl_fixed_t y)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.time = time;
		event.x = wl_fixed_to_double(sx);
		event.y = wl_fixed_to_double(sy);
		event.gx = wl_fixed_to_double(x);
		event.gy = wl_fixed_to_double(y);

		canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_MOTION_EVENT, &event);
	}
}

static void touch_handle_pressure(void *data, struct nemo_touch *touch, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t p)
{
	if (surface != NULL) {
		struct nemocanvas *canvas = (struct nemocanvas *)wl_surface_get_user_data(surface);
		struct nemoevent event;

		event.device = id;
		event.time = time;
		event.p = wl_fixed_to_double(p);

		canvas->dispatch_event(canvas, NEMOTOOL_TOUCH_PRESSURE_EVENT, &event);
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
	touch_handle_pressure,
	touch_handle_frame,
	touch_handle_cancel
};

void nemotool_touch_bypass(struct nemotool *tool, int32_t id, float x, float y)
{
	nemo_touch_bypass(tool->touch, id, wl_fixed_from_double(x), wl_fixed_from_double(y));
}

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
		tool->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, version);
		tool->compositor_version = version;
	} else if (strcmp(interface, "wl_subcompositor") == 0) {
		tool->subcompositor = wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
	} else if (strcmp(interface, "wl_output") == 0) {
		nemooutput_register(tool, id);
	} else if (strcmp(interface, "nemo_seat") == 0) {
		tool->seat = wl_registry_bind(registry, id, &nemo_seat_interface, 1);
		nemo_seat_add_listener(tool->seat, &seat_listener, data);
	} else if (strcmp(interface, "nemo_sound") == 0) {
		tool->sound = wl_registry_bind(registry, id, &nemo_sound_interface, 1);
	} else if (strcmp(interface, "wl_shm") == 0) {
		tool->shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
		wl_shm_add_listener(tool->shm, &shm_listener, data);
	} else if (strcmp(interface, "nemo_shell") == 0) {
		tool->shell = wl_registry_bind(registry, id, &nemo_shell_interface, 1);
		nemo_shell_use_unstable_version(tool->shell, 1);
		nemo_shell_add_listener(tool->shell, &nemo_shell_listener, data);
	} else if (strcmp(interface, "nemo_client") == 0) {
		tool->client = wl_registry_bind(registry, id, &nemo_client_interface, 1);
	} else if (strcmp(interface, "presentation") == 0) {
		tool->presentation = wl_registry_bind(registry, id, &presentation_interface, 1);
		presentation_add_listener(tool->presentation, &presentation_listener, data);
	}

	if (tool->dispatch_global != NULL)
		tool->dispatch_global(tool, id, interface, version);
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

static void nemotool_dispatch_wayland_display(void *data, const char *events)
{
	struct nemotool *tool = (struct nemotool *)data;
	struct epoll_event ep;
	int ret;

	if (strchr(events, 'e') != NULL || strchr(events, 'h') != NULL) {
		tool->running = 0;
		return;
	}

	if (strchr(events, 'r') != NULL) {
		ret = wl_display_dispatch(tool->display);
		if (ret == -1) {
			tool->running = 0;
			return;
		}
	}

	if (strchr(events, 'w') != NULL) {
		ret = wl_display_flush(tool->display);
		if (ret == 0) {
			nemotool_change_source(tool, tool->display_fd, "reh");
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

	nemotool_watch_source(tool, tool->display_fd, "reh", nemotool_dispatch_wayland_display, tool);

	tool->registry = wl_display_get_registry(tool->display);
	wl_registry_add_listener(tool->registry, &registry_listener, tool);

	wl_display_dispatch(tool->display);
}

void nemotool_disconnect_wayland(struct nemotool *tool)
{
	wl_display_disconnect(tool->display);

	tool->display = NULL;
}

static inline struct nemosource *nemotool_create_source(struct nemotool *tool, int fd)
{
	struct nemosource *source;

	source = (struct nemosource *)malloc(sizeof(struct nemosource));
	if (source == NULL)
		return NULL;
	memset(source, 0, sizeof(struct nemosource));

	source->fd = fd;

	nemolist_insert(&tool->source_list, &source->link);

	return source;
}

static inline void nemotool_destroy_source(struct nemosource *source)
{
	nemolist_remove(&source->link);

	free(source);
}

static inline struct nemosource *nemotool_search_source(struct nemotool *tool, int fd)
{
	struct nemosource *source;

	nemolist_for_each(source, &tool->source_list, link) {
		if (source->fd == fd)
			return source;
	}

	return NULL;
}

void nemotool_dispatch(struct nemotool *tool)
{
	struct nemosource *source;
	struct nemoidle *idle;
	struct epoll_event ep[16];
	int i, count, ret;

	while (!nemolist_empty(&tool->idle_list)) {
		idle = (struct nemoidle *)container_of(tool->idle_list.prev, struct nemoidle, link);

		idle->dispatch(idle->data);

		nemolist_remove(&idle->link);

		free(idle);
	}

	if (tool->display != NULL) {
		wl_display_dispatch_pending(tool->display);

		ret = wl_display_flush(tool->display);
		if (ret < 0 && errno == EAGAIN) {
			nemotool_change_source(tool, tool->display_fd, "rweh");
		} else if (ret < 0) {
			return;
		}
	}

	count = epoll_wait(tool->epoll_fd, ep, ARRAY_LENGTH(ep), -1);
	for (i = 0; i < count; i++) {
		source = ep[i].data.ptr;
		if (source->fd >= 0) {
			char events[8];
			int nevents = 0;

			if (ep[i].events & EPOLLIN)
				events[nevents++] = 'r';
			if (ep[i].events & EPOLLOUT)
				events[nevents++] = 'w';
			if (ep[i].events & EPOLLERR)
				events[nevents++] = 'e';
			if (ep[i].events & EPOLLHUP)
				events[nevents++] = 'h';
			events[nevents] = '\0';

			source->dispatch(source->data, events);
		} else {
			nemotool_destroy_source(source);
		}
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

static void sync_handle_done(void *data, struct wl_callback *callback, uint32_t serial)
{
	int *done = (int *)data;

	*done = 1;

	wl_callback_destroy(callback);
}

static const struct wl_callback_listener sync_listener = {
	.done = sync_handle_done
};

int nemotool_roundtrip(struct nemotool *tool)
{
	struct wl_callback *callback;
	int done = 0;
	int r = 0;

	callback = wl_display_sync(tool->display);
	wl_callback_add_listener(callback, &sync_listener, &done);

	while (r != -1 && done == 0)
		r = wl_display_dispatch(tool->display);

	if (done == 0)
		wl_callback_destroy(callback);

	return r;
}

void nemotool_exit(struct nemotool *tool)
{
	tool->running = 0;
}

int nemotool_watch_source(struct nemotool *tool, int fd, const char *events, nemotool_dispatch_source_t dispatch, void *data)
{
	struct nemosource *source;
	struct epoll_event ep;

	source = nemotool_create_source(tool, fd);
	if (source == NULL)
		return -1;

	source->dispatch = dispatch;
	source->data = data;
	source->fd = fd;

	ep.events = 0x0;
	if (strchr(events, 'r') != NULL)
		ep.events |= EPOLLIN;
	if (strchr(events, 'w') != NULL)
		ep.events |= EPOLLOUT;
	if (strchr(events, 'e') != NULL)
		ep.events |= EPOLLERR;
	if (strchr(events, 'u') != NULL)
		ep.events |= EPOLLHUP;

	ep.data.ptr = source;
	epoll_ctl(tool->epoll_fd, EPOLL_CTL_ADD, fd, &ep);

	return 0;
}

void nemotool_unwatch_source(struct nemotool *tool, int fd)
{
	struct nemosource *source;

	source = nemotool_search_source(tool, fd);
	if (source != NULL) {
		source->fd = -1;
	}

	epoll_ctl(tool->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void nemotool_change_source(struct nemotool *tool, int fd, const char *events)
{
	struct nemosource *source;

	source = nemotool_search_source(tool, fd);
	if (source != NULL) {
		struct epoll_event ep;

		ep.events = 0x0;
		if (strchr(events, 'r') != NULL)
			ep.events |= EPOLLIN;
		if (strchr(events, 'w') != NULL)
			ep.events |= EPOLLOUT;
		if (strchr(events, 'e') != NULL)
			ep.events |= EPOLLERR;
		if (strchr(events, 'u') != NULL)
			ep.events |= EPOLLHUP;

		ep.data.ptr = source;
		epoll_ctl(tool->epoll_fd, EPOLL_CTL_MOD, fd, &ep);
	}
}

int nemotool_get_fd(struct nemotool *tool)
{
	return tool->epoll_fd;
}

static struct nemotool *s_nemotool = NULL;

struct nemotool *nemotool_get_instance(void)
{
	return s_nemotool;
}

struct nemotool *nemotool_create(void)
{
	struct nemotool *tool;

	s_nemotool = tool = (struct nemotool *)malloc(sizeof(struct nemotool));
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

	nemolist_init(&tool->source_list);

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

	if (tool->display != NULL)
		nemotool_disconnect_wayland(tool);
	if (tool->eglcontext != NULL)
		nemotool_disconnect_egl(tool);

	nemolist_remove(&tool->global_list);
	nemolist_remove(&tool->output_list);

	nemolist_remove(&tool->idle_list);

	nemolist_remove(&tool->source_list);

	close(tool->epoll_fd);

	free(tool);
}

int nemotool_dispatch_idle(struct nemotool *tool, nemotool_dispatch_idle_t dispatch, void *data)
{
	struct nemoidle *idle;

	idle = (struct nemoidle *)malloc(sizeof(struct nemoidle));
	if (idle == NULL)
		return -1;
	memset(idle, 0, sizeof(struct nemoidle));

	idle->dispatch = dispatch;
	idle->data = data;

	nemolist_insert(&tool->idle_list, &idle->link);

	return 0;
}

void nemotool_client_alive(struct nemotool *tool, uint32_t timeout)
{
	nemo_client_alive(tool->client, timeout);
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
