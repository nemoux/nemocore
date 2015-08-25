#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <cairo.h>
#include <pixman.h>
#include <wayland-client.h>
#include <wayland-cursor.h>

#define	WL_HIDE_DEPRECATED
#include <wayland-server.h>
#include <wayland-presentation-timing-server-protocol.h>

#include <waylandnode.h>
#include <compz.h>
#include <screen.h>
#include <pixmanrenderer.h>
#include <seat.h>
#include <pointer.h>
#include <keyboard.h>
#include <touch.h>
#include <nemomisc.h>
#include <nemolog.h>

#ifdef NEMOUX_WITH_EGL
#include <wayland-egl.h>

#include <glrenderer.h>
#endif

struct waylandbuffer {
	struct waylandscreen *screen;
	struct wl_list link;
	struct wl_list free_link;

	struct wl_buffer *buffer;
	void *data;
	size_t size;
	pixman_region32_t damage;

	pixman_image_t *image;
};

static void wayland_destroy_shm_buffer(struct waylandbuffer *buffer);

static void buffer_release(void *data, struct wl_buffer *wbuffer)
{
	struct waylandbuffer *buffer = (struct waylandbuffer *)data;

	if (buffer->screen) {
		wl_list_insert(&buffer->screen->shm.free_buffers, &buffer->free_link);
	} else {
		wayland_destroy_shm_buffer(buffer);
	}
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static struct waylandbuffer *wayland_create_shm_buffer(struct waylandscreen *screen)
{
	struct nemocompz *compz = screen->base.compz;
	struct waylandnode *node = screen->node;
	struct waylandbuffer *buffer;
	struct wl_shm_pool *pool;
	void *data;
	int width, height, stride;
	int fd;

	if (!wl_list_empty(&screen->shm.free_buffers)) {
		buffer = (struct waylandbuffer *)container_of(screen->shm.free_buffers.next, struct waylandbuffer, free_link);
		wl_list_remove(&buffer->free_link);
		wl_list_init(&buffer->free_link);

		return buffer;
	}

	width = screen->base.current_mode->width;
	height = screen->base.current_mode->height;

	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

	fd = os_create_anonymous_file(height * stride);
	if (fd < 0) {
		nemolog_error("WAYLAND", "failed to create anonymous file buffer\n");
		return NULL;
	}

	data = mmap(NULL, height * stride, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		nemolog_error("WAYLAND", "failed to mmap memory for buffer\n");
		goto err1;
	}

	buffer = (struct waylandbuffer *)malloc(sizeof(struct waylandbuffer));
	if (buffer == NULL)
		goto err2;
	memset(buffer, 0, sizeof(struct waylandbuffer));

	buffer->screen = screen;

	wl_list_init(&buffer->free_link);
	wl_list_insert(&screen->shm.buffers, &buffer->link);

	pixman_region32_init_rect(&buffer->damage, 0, 0, screen->base.width, screen->base.height);

	buffer->data = data;
	buffer->size = height * stride;

	pool = wl_shm_create_pool(node->parent.shm, fd, buffer->size);
	if (pool == NULL)
		goto err3;

	buffer->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
	wl_shm_pool_destroy(pool);
	close(fd);

	memset(data, 0, buffer->size);

	buffer->image = pixman_image_create_bits(PIXMAN_a8r8g8b8, width, height, (uint32_t *)data, stride);

	return buffer;

err3:
	pixman_region32_fini(&buffer->damage);

	free(buffer);

err2:
	munmap(data, height * stride);

err1:
	close(fd);

	return NULL;
}

static void wayland_destroy_shm_buffer(struct waylandbuffer *buffer)
{
	pixman_image_unref(buffer->image);

	wl_buffer_destroy(buffer->buffer);
	munmap(buffer->data, buffer->size);

	pixman_region32_fini(&buffer->damage);

	wl_list_remove(&buffer->link);
	wl_list_remove(&buffer->free_link);

	free(buffer);
}

static void wayland_attach_shm_buffer(struct waylandbuffer *buffer)
{
	pixman_region32_t damage;
	pixman_box32_t *rects;
	int i, nrects;

	pixman_region32_init(&damage);
	wayland_transform_region(
			buffer->screen->base.width,
			buffer->screen->base.height,
			WL_OUTPUT_TRANSFORM_NORMAL,
			1,
			&buffer->damage, &damage);

	rects = pixman_region32_rectangles(&damage, &nrects);

	wl_surface_attach(buffer->screen->parent.surface, buffer->buffer, 0, 0);

	for (i = 0; i < nrects; i++) {
		wl_surface_damage(buffer->screen->parent.surface,
				rects[i].x1, rects[i].y1,
				rects[i].x2 - rects[i].x1, rects[i].y2 - rects[i].y1);
	}
}

static void pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
{
	struct waylandnode *node = (struct waylandnode *)data;

	node->parent.pointer_serial = serial;

	wl_pointer_set_cursor(node->parent.pointer, serial, NULL, 0, 0);
}

static void pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface)
{
	struct waylandnode *node = (struct waylandnode *)data;
}

static void pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	struct waylandnode *node = (struct waylandnode *)data;

	nemopointer_notify_motion_absolute(node->pointer, time, wl_fixed_to_double(x), wl_fixed_to_double(y));
}

static void pointer_handle_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct waylandnode *node = (struct waylandnode *)data;

	nemopointer_notify_button(node->pointer, time, button, state);
}

static void pointer_handle_axis(void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	struct waylandnode *node = (struct waylandnode *)data;

	nemopointer_notify_axis(node->pointer, time, axis, wl_fixed_to_double(value));
}

static const struct wl_pointer_listener pointer_listener = {
	pointer_handle_enter,
	pointer_handle_leave,
	pointer_handle_motion,
	pointer_handle_button,
	pointer_handle_axis
};

static void keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size)
{
}

static void keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
	struct waylandnode *node = (struct waylandnode *)data;

	node->parent.keyboard_serial = serial;
}

static void keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface)
{
}

static void keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	struct waylandnode *node = (struct waylandnode *)data;

	nemokeyboard_notify_key(node->keyboard, time, key, state);
}

static void keyboard_handle_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

static const struct wl_keyboard_listener keyboard_listener = {
	keyboard_handle_keymap,
	keyboard_handle_enter,
	keyboard_handle_leave,
	keyboard_handle_key,
	keyboard_handle_modifiers
};

static void seat_handle_capabilities(void *data, struct wl_seat *seat, enum wl_seat_capability caps)
{
	struct waylandnode *node = (struct waylandnode *)data;
	struct nemocompz *compz = node->base.compz;

	if ((caps & WL_SEAT_CAPABILITY_POINTER) && node->parent.pointer == NULL) {
		node->parent.pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(node->parent.pointer, &pointer_listener, node);
		node->pointer = nemopointer_create(compz->seat, NULL);
	} else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && node->parent.pointer != NULL) {
		wl_pointer_destroy(node->parent.pointer);
		node->parent.pointer = NULL;
	}

	if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && node->parent.keyboard == NULL) {
		node->parent.keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(node->parent.keyboard, &keyboard_listener, node);
		node->keyboard = nemokeyboard_create(compz->seat, NULL);
	} else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && node->parent.keyboard != NULL) {
		wl_keyboard_destroy(node->parent.keyboard);
		node->parent.keyboard = NULL;
	}
}

static const struct wl_seat_listener seat_listener = {
	seat_handle_capabilities
};

static void wayland_bind_seat(struct waylandnode *node, uint32_t name)
{
	node->parent.seat = wl_registry_bind(node->parent.registry, name, &wl_seat_interface, 1);
	wl_seat_add_listener(node->parent.seat, &seat_listener, node);
}

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	struct waylandnode *node = (struct waylandnode *)data;

	if (!strcmp(interface, "wl_compositor")) {
		node->parent.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
	} else if (!strcmp(interface, "wl_shell")) {
		node->parent.shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
	} else if (!strcmp(interface, "wl_seat")) {
		wayland_bind_seat(node, name);
	} else if (!strcmp(interface, "wl_output")) {
	} else if (!strcmp(interface, "wl_shm")) {
		node->parent.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
	struct waylandnode *node = (struct waylandnode *)data;
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static int wayland_dispatch_display_event(int fd, uint32_t mask, void *data)
{
	struct waylandnode *node = (struct waylandnode *)data;
	int count = 0;

	if ((mask & WL_EVENT_HANGUP) || (mask & WL_EVENT_ERROR)) {
		wl_display_terminate(node->parent.display);
		return 0;
	}

	if (mask & WL_EVENT_READABLE)
		count = wl_display_dispatch(node->parent.display);
	if (mask & WL_EVENT_WRITABLE)
		wl_display_flush(node->parent.display);

	if (mask == 0) {
		count = wl_display_dispatch_pending(node->parent.display);
		wl_display_flush(node->parent.display);
	}

	return count;
}

static void frame_done(void *data, struct wl_callback *cb, uint32_t time)
{
	struct waylandscreen *screen = (struct waylandscreen *)data;

	wl_callback_destroy(cb);
	nemoscreen_finish_frame(&screen->base, time / 1000, (time % 1000) * 1000, 0x0);
}

static const struct wl_callback_listener frame_listener = {
	frame_done
};

static void wayland_screen_repaint(struct nemoscreen *base)
{
	struct waylandscreen *screen = (struct waylandscreen *)container_of(base, struct waylandscreen, base);
	struct nemocompz *compz = screen->base.compz;
	struct wl_callback *cb;

	if (screen->initial_frame) {
		struct waylandbuffer *buffer;

		buffer = wayland_create_shm_buffer(screen);

#ifdef NEMOUX_WITH_EGL
		if (screen->parent.egl_window)
			buffer->screen = NULL;
#endif

		wl_surface_attach(screen->parent.surface, buffer->buffer, 0, 0);
		wl_surface_damage(screen->parent.surface, 0, 0, screen->base.current_mode->width, screen->base.current_mode->height);

		screen->initial_frame = 0;
	}

	cb = wl_surface_frame(screen->parent.surface);
	wl_callback_add_listener(cb, &frame_listener, screen);
	wl_surface_commit(screen->parent.surface);
	wl_display_flush(screen->node->parent.display);
}

static int wayland_screen_repaint_frame_pixman(struct nemoscreen *base, pixman_region32_t *damage)
{
	struct waylandscreen *screen = (struct waylandscreen *)container_of(base, struct waylandscreen, base);
	struct waylandnode *node = screen->node;
	struct nemocompz *compz = screen->base.compz;
	struct waylandbuffer *buffer;
	struct wl_callback *cb;

	wl_list_for_each(buffer, &screen->shm.buffers, link)
		pixman_region32_union(&buffer->damage, &buffer->damage, damage);

	buffer = wayland_create_shm_buffer(screen);

	pixmanrenderer_set_screen_buffer(node->base.pixman, &screen->base, buffer->image);

	node->base.pixman->repaint_screen(node->base.pixman, &screen->base, &buffer->damage);

	wayland_attach_shm_buffer(buffer);

	cb = wl_surface_frame(screen->parent.surface);
	wl_callback_add_listener(cb, &frame_listener, screen);
	wl_surface_commit(screen->parent.surface);
	wl_display_flush(node->parent.display);

	pixman_region32_fini(&buffer->damage);
	pixman_region32_init(&buffer->damage);

	pixman_region32_subtract(&screen->base.damage, &screen->base.damage, damage);

	return 0;
}

static int wayland_screen_repaint_frame_gl(struct nemoscreen *base, pixman_region32_t *damage)
{
	struct waylandscreen *screen = (struct waylandscreen *)container_of(base, struct waylandscreen, base);
	struct waylandnode *node = screen->node;
	struct nemocompz *compz = screen->base.compz;
	struct wl_callback *cb;

	cb = wl_surface_frame(screen->parent.surface);
	wl_callback_add_listener(cb, &frame_listener, screen);

	node->base.opengl->repaint_screen(node->base.opengl, &screen->base, damage);

	pixman_region32_subtract(&screen->base.damage, &screen->base.damage, damage);

	return 0;
}

static int wayland_screen_read_pixels(struct nemoscreen *base, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	return 0;
}

static int wayland_prepare_pixman(struct waylandnode *node)
{
	node->base.pixman = pixmanrenderer_create(&node->base);
	if (node->base.pixman == NULL)
		return -1;

	return 0;
}

static void wayland_finish_pixman(struct waylandnode *node)
{
	if (node->base.pixman == NULL)
		return;

	pixmanrenderer_destroy(node->base.pixman);
}

static int wayland_prepare_pixman_screen(struct waylandscreen *screen)
{
	struct waylandnode *node = screen->node;

	if (pixmanrenderer_prepare_screen(node->base.pixman, &screen->base) < 0) {
		nemolog_error("WAYLAND", "failed to prepare pixman renderer screen\n");
		return -1;
	}

	return 0;
}

static void wayland_finish_pixman_screen(struct waylandscreen *screen)
{
	struct waylandnode *node = screen->node;

	pixmanrenderer_finish_screen(node->base.pixman, &screen->base);
}

#ifdef NEMOUX_WITH_EGL
static int wayland_prepare_egl(struct waylandnode *node)
{
	node->base.opengl = glrenderer_create(&node->base, (EGLNativeDisplayType)node->parent.display, 0, NULL);
	if (node->base.opengl == NULL)
		return -1;

	return 0;
}

static void wayland_finish_egl(struct waylandnode *node)
{
	if (node->base.opengl == NULL)
		return;

	glrenderer_destroy(node->base.opengl);
}

static int wayland_prepare_egl_screen(struct waylandscreen *screen)
{
	struct waylandnode *node = screen->node;

	screen->parent.egl_window = wl_egl_window_create(screen->parent.surface,
			screen->base.current_mode->width,
			screen->base.current_mode->height);
	if (!screen->parent.egl_window) {
		nemolog_error("WAYLAND", "failed to create wayland egl window\n");
		return -1;
	}

	if (glrenderer_prepare_screen(node->base.opengl, &screen->base, (EGLNativeWindowType)screen->parent.egl_window, 0, NULL) < 0)
		goto err1;

	return 0;

err1:
	wl_egl_window_destroy(screen->parent.egl_window);

	return -1;
}

static void wayland_finish_egl_screen(struct waylandscreen *screen)
{
	wl_egl_window_destroy(screen->parent.egl_window);
}
#endif

static void wayland_screen_destroy(struct nemoscreen *base)
{
	struct waylandscreen *screen = (struct waylandscreen *)container_of(base, struct waylandscreen, base);

#ifdef NEMOUX_WITH_EGL
	if (base->compz->use_pixman != 0) {
		wayland_finish_pixman_screen(screen);
	} else if (screen->base.use_pixman != 0) {
		wayland_finish_pixman_screen(screen);
		wayland_finish_egl_screen(screen);
	} else {
		wayland_finish_egl_screen(screen);
	}
#else
	wayland_finish_pixman_screen(screen);
#endif

	wl_list_remove(&base->link);

	nemoscreen_finish(base);

	free(screen);
}

static int wayland_screen_switch_mode(struct nemoscreen *base, struct nemomode *mode)
{
	return 0;
}

static void wayland_screen_set_gamma(struct nemoscreen *base, uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b)
{
}

static void shell_surface_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

static void shell_surface_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
}

static void shell_surface_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	shell_surface_ping,
	shell_surface_configure,
	shell_surface_popup_done
};

static int wayland_create_screen(struct waylandnode *node, int x, int y, int width, int height)
{
	struct nemocompz *compz = node->base.compz;
	struct waylandscreen *screen;
	const char *renderer;

	screen = (struct waylandscreen *)malloc(sizeof(struct waylandscreen));
	if (screen == NULL)
		return -1;
	memset(screen, 0, sizeof(struct waylandscreen));

	screen->base.compz = compz;
	screen->base.node = &node->base;
	screen->base.screenid = 0;
	screen->base.make = "wayland";
	screen->base.model = "none";
	screen->base.subpixel = WL_OUTPUT_SUBPIXEL_UNKNOWN;

	wl_list_init(&screen->base.mode_list);

	screen->node = node;
	screen->initial_frame = 1;

	screen->parent.surface = wl_compositor_create_surface(node->parent.compositor);
	wl_surface_set_user_data(screen->parent.surface, screen);

	if (node->parent.shell) {
		screen->parent.shell_surface = wl_shell_get_shell_surface(node->parent.shell, screen->parent.surface);
		wl_shell_surface_add_listener(screen->parent.shell_surface, &shell_surface_listener, screen);
		wl_shell_surface_set_toplevel(screen->parent.shell_surface);
	}

	screen->mode.flags = WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED;
	screen->mode.width = width;
	screen->mode.height = height;
	screen->mode.refresh = 60000;
	screen->base.current_mode = &screen->mode;
	wl_list_init(&screen->base.mode_list);
	wl_list_insert(&screen->base.mode_list, &screen->mode.link);

	wl_list_init(&screen->shm.buffers);
	wl_list_init(&screen->shm.free_buffers);

	renderer = nemoscreen_get_config(compz, node->base.nodeid, screen->base.screenid, "renderer");
	if (renderer != NULL && strcmp(renderer, "pixman") == 0)
		screen->base.use_pixman = 1;
	else
		screen->base.use_pixman = 0;

#ifdef NEMOUX_WITH_EGL
	if (compz->use_pixman == 0 && screen->base.use_pixman != 0) {
		if (wayland_prepare_pixman(node) < 0) {
			nemolog_error("WAYLAND", "failed to prepare pixman renderer\n");
			goto err1;
		}
	}
#endif

	nemoscreen_prepare(&screen->base, x, y, width, height, width, height, 0, width, height, 1);

#ifdef NEMOUX_WITH_EGL
	if (compz->use_pixman != 0) {
		if (wayland_prepare_pixman_screen(screen) < 0) {
			nemolog_error("WAYLAND", "failed to prepare pixman screen\n");
			goto err1;
		}
		screen->base.repaint_frame = wayland_screen_repaint_frame_pixman;
	} else if (screen->base.use_pixman != 0) {
		if (wayland_prepare_pixman_screen(screen) < 0) {
			nemolog_error("WAYLAND", "failed to prepare pixman screen\n");
			goto err1;
		}
		if (wayland_prepare_egl_screen(screen) < 0) {
			nemolog_error("WAYLAND", "failed to prepare egl screen\n");
			goto err1;
		}
	} else {
		if (wayland_prepare_egl_screen(screen) < 0) {
			nemolog_error("WAYLAND", "failed to prepare egl screen\n");
			goto err1;
		}
		screen->base.repaint_frame = wayland_screen_repaint_frame_gl;
	}
#else
	if (wayland_prepare_pixman_screen(screen) < 0) {
		nemolog_error("WAYLAND", "failed to prepare pixman screen\n");
		goto err1;
	}
	screen->base.repaint_frame = wayland_screen_repaint_frame_pixman;
#endif

	screen->base.repaint = wayland_screen_repaint;
	screen->base.read_pixels = wayland_screen_read_pixels;
	screen->base.destroy = wayland_screen_destroy;
	screen->base.set_dpms = NULL;
	screen->base.switch_mode = wayland_screen_switch_mode;
	screen->base.set_gamma = wayland_screen_set_gamma;

	wl_list_insert(compz->screen_list.prev, &screen->base.link);

	nemoscreen_update_geometry(&screen->base);

	return 0;

err1:
	free(screen);

	return -1;
}

struct waylandnode *wayland_create_node(struct nemocompz *compz, const char *name)
{
	struct waylandnode *node;
	int fd;

	node = (struct waylandnode *)malloc(sizeof(struct waylandnode));
	if (node == NULL)
		return NULL;
	memset(node, 0, sizeof(struct waylandnode));

	node->base.compz = compz;
	node->base.nodeid = 0;
	node->base.pixman = NULL;
	node->base.opengl = NULL;

	rendernode_prepare(&node->base);

	node->parent.display = wl_display_connect(name);
	if (node->parent.display == NULL) {
		nemolog_error("WAYLAND", "failed to connect wayland display\n");
		goto err1;
	}

	node->parent.registry = wl_display_get_registry(node->parent.display);
	wl_registry_add_listener(node->parent.registry, &registry_listener, node);
	wl_display_roundtrip(node->parent.display);

	fd = wl_display_get_fd(node->parent.display);
	node->display_source = wl_event_loop_add_fd(compz->loop,
			fd,
			WL_EVENT_READABLE,
			wayland_dispatch_display_event,
			node);
	if (node->display_source == NULL)
		goto err2;

	wl_event_source_check(node->display_source);

#ifdef NEMOUX_WITH_EGL
	if (compz->use_pixman != 0) {
		if (wayland_prepare_pixman(node) < 0) {
			nemolog_error("WAYLAND", "failed to prepare pixman renderer\n");
			goto err2;
		}
	} else {
		if (wayland_prepare_egl(node) < 0) {
			nemolog_error("WAYLAND", "failed to prepare egl renderer\n");
			goto err2;
		}
	}
#else
	if (wayland_prepare_pixman(node) < 0) {
		nemolog_error("WAYLAND", "failed to prepare pixman renderer\n");
		goto err2;
	}
#endif

	wayland_create_screen(node, 0, 0, 800, 600);

	wl_list_insert(compz->render_list.prev, &node->base.link);

	return node;

err2:
	wl_display_disconnect(node->parent.display);

err1:
	wayland_destroy_node(node);

	return NULL;
}

void wayland_destroy_node(struct waylandnode *node)
{
	struct nemocompz *compz = node->base.compz;

	wl_list_remove(&node->base.link);

#ifdef NEMOUX_WITH_EGL
	wayland_finish_pixman(node);
	wayland_finish_egl(node);
#else
	wayland_finish_pixman(node);
#endif

	wl_display_disconnect(node->parent.display);

	rendernode_finish(&node->base);

	free(node);
}
