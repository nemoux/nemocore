#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <linux/input.h>

#include <pixman.h>
#include <cairo.h>

#include <wayland-client.h>
#include <wayland-nemo-seat-client-protocol.h>
#include <wayland-nemo-shell-client-protocol.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemooutput.h>
#include <oshelper.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

static void nemo_surface_handle_configure(void *data, struct nemo_surface *surface, int32_t width, int32_t height, int32_t fixed)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	if (canvas->dispatch_resize != NULL)
		canvas->dispatch_resize(canvas, width, height, fixed);
}

static void nemo_surface_handle_transform(void *data, struct nemo_surface *surface, int32_t visible)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	if (canvas->dispatch_transform != NULL)
		canvas->dispatch_transform(canvas, visible);
}

static void nemo_surface_handle_fullscreen(void *data, struct nemo_surface *surface, int32_t active, int32_t opaque)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	if (canvas->dispatch_fullscreen != NULL)
		canvas->dispatch_fullscreen(canvas, active, opaque);
}

static const struct nemo_surface_listener nemo_surface_listener = {
	nemo_surface_handle_configure,
	nemo_surface_handle_transform,
	nemo_surface_handle_fullscreen
};

static void buffer_release(void *data, struct wl_buffer *buffer)
{
	struct nemobuffer *nemobuffer = (struct nemobuffer *)data;

	nemobuffer->busy = 0;
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static int nemocanvas_get_shm_buffer(struct wl_shm *shm, struct nemobuffer *buffer, int width, int height, uint32_t format)
{
	struct wl_shm_pool *pool;
	int fd, size, stride;
	void *data;

	stride = width * 4;
	size = stride * height;

	fd = os_create_anonymous_file(size);
	if (fd < 0)
		return -1;

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		close(fd);
		return -1;
	}

	pool = wl_shm_create_pool(shm, fd, size);
	buffer->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, format);
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
	wl_shm_pool_destroy(pool);
	close(fd);

	buffer->shm_data = data;
	buffer->shm_size = size;
	buffer->width = width;
	buffer->height = height;

	return 0;
}

static struct nemobuffer *nemocanvas_get_next_buffer(struct nemocanvas *canvas)
{
	struct nemobuffer *buffer = NULL;
	int i, ret = 0;

	if (canvas->buffers[0].busy == 0) {
		buffer = &canvas->buffers[0];
	} else if (canvas->buffers[1].busy == 0) {
		buffer = &canvas->buffers[1];
	} else {
retry:
		for (i = 0; i < canvas->nextras; i++) {
			if (canvas->extras[i].busy == 0) {
				buffer = &canvas->extras[i];
				break;
			}
		}

		if (buffer == NULL) {
			canvas->extras = (struct nemobuffer *)realloc(canvas->extras, sizeof(struct nemobuffer) * (canvas->nextras + 4));
			if (canvas->extras == NULL)
				return NULL;
			memset(&canvas->extras[canvas->nextras], 0, sizeof(struct nemobuffer) * 4);

			canvas->nextras = canvas->nextras + 4;

			goto retry;
		}
	}

	if (buffer == NULL) {
		buffer = &canvas->buffers[0];
	}

	if ((buffer->buffer != NULL) &&
			(buffer->width != canvas->width || buffer->height != canvas->height)) {
		wl_buffer_destroy(buffer->buffer);

		munmap(buffer->shm_data, buffer->shm_size);

		buffer->busy = 0;
		buffer->buffer = NULL;
	}

	if (buffer->buffer == NULL) {
		struct wl_shm *shm = canvas->tool->shm;

		ret = nemocanvas_get_shm_buffer(shm, buffer, canvas->width, canvas->height, WL_SHM_FORMAT_ARGB8888);
		if (ret < 0)
			return NULL;

		memset(buffer->shm_data, 0xff, canvas->width * canvas->height * 4);
	}

	return buffer;
}

static void frame_handle_done(void *data, struct wl_callback *callback, uint32_t time)
{
	wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
	frame_handle_done
};

int nemocanvas_buffer(struct nemocanvas *canvas)
{
	canvas->buffer = nemocanvas_get_next_buffer(canvas);
	if (canvas->buffer == NULL)
		return -1;

	return 0;
}

void nemocanvas_damage(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_union_rect(&canvas->damage, &canvas->damage,
			x, y,
			width > 0 ? width : canvas->width,
			height > 0 ? height : canvas->height);
}

void nemocanvas_damage_region(struct nemocanvas *canvas, pixman_region32_t *region)
{
	pixman_region32_union(&canvas->damage, &canvas->damage, region);
}

void nemocanvas_commit(struct nemocanvas *canvas)
{
	struct wl_callback *callback;
	pixman_box32_t *rects;
	int nrects, i;

	wl_surface_attach(canvas->surface, canvas->buffer->buffer, 0, 0);

	rects = pixman_region32_rectangles(&canvas->damage, &nrects);

	for (i = 0; i < nrects; i++) {
		wl_surface_damage(canvas->surface,
				rects[i].x1,
				rects[i].y1,
				rects[i].x2 - rects[i].x1,
				rects[i].y2 - rects[i].y1);
	}

	callback = wl_surface_frame(canvas->surface);
	wl_callback_add_listener(callback, &frame_listener, canvas);
	wl_surface_commit(canvas->surface);

	canvas->buffer->busy = 1;
	pixman_region32_clear(&canvas->damage);
}

void nemocanvas_opaque(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemotool *tool = canvas->tool;
	struct wl_region *region;

	region = wl_compositor_create_region(tool->compositor);
	wl_region_add(region, 0, 0, width, height);

	wl_surface_set_opaque_region(canvas->surface, region);

	wl_region_destroy(region);
}

static void surface_enter(void *data, struct wl_surface *surface, struct wl_output *_output)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;
	struct nemooutput *output;

	if (canvas->dispatch_screen != NULL) {
		output = nemooutput_find(canvas->tool, _output);
		if (output == NULL)
			return;

		canvas->dispatch_screen(canvas, output->x, output->y, output->width, output->height, output->mmwidth, output->mmheight, 0);
	}
}

static void surface_leave(void *data, struct wl_surface *surface, struct wl_output *_output)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;
	struct nemooutput *output;

	if (canvas->dispatch_screen != NULL) {
		output = nemooutput_find(canvas->tool, _output);
		if (output == NULL)
			return;

		canvas->dispatch_screen(canvas, output->x, output->y, output->width, output->height, output->mmwidth, output->mmheight, 1);
	}
}

static const struct wl_surface_listener surface_listener = {
	surface_enter,
	surface_leave
};

static void nemocanvas_dispatch_frame_event(struct nemotask *task, uint32_t events)
{
	struct nemocanvas *canvas = (struct nemocanvas *)container_of(task, struct nemocanvas, frame_task);

	nemocanvas_dispatch_frame(canvas);
}

static void nemocanvas_dispatch_frame_timer(struct nemotimer *timer, void *data)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;
	uint64_t secs;
	uint32_t nsecs;

	canvas->framefeed = 0;

	time_current_nsecs(&secs, &nsecs);

	canvas->dispatch_frame(canvas, secs, nsecs);
}

struct nemocanvas *nemocanvas_create(struct nemotool *tool)
{
	struct nemocanvas *canvas;

	canvas = (struct nemocanvas *)malloc(sizeof(struct nemocanvas));
	if (canvas == NULL)
		return NULL;
	memset(canvas, 0, sizeof(struct nemocanvas));

	canvas->eventfd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
	if (canvas->eventfd < 0)
		goto err1;
	canvas->frame_task.dispatch = nemocanvas_dispatch_frame_event;

	nemotool_watch_fd(tool, canvas->eventfd, EPOLLIN, &canvas->frame_task);

	nemosignal_init(&canvas->destroy_signal);

	nemolist_init(&canvas->repaint_task.link);

	pixman_region32_init(&canvas->damage);

	canvas->tool = tool;

	canvas->surface = wl_compositor_create_surface(tool->compositor);
	wl_surface_add_listener(canvas->surface, &surface_listener, canvas);
	wl_surface_set_user_data(canvas->surface, canvas);

	canvas->frametimer = nemotimer_create(tool);
	nemotimer_set_callback(canvas->frametimer, nemocanvas_dispatch_frame_timer);
	nemotimer_set_userdata(canvas->frametimer, canvas);

	return canvas;

err1:
	free(canvas);

	return NULL;
}

void nemocanvas_destroy(struct nemocanvas *canvas)
{
	struct nemobuffer *buffer;
	int i;

	nemosignal_emit(&canvas->destroy_signal, canvas);

	nemotimer_destroy(canvas->frametimer);

	for (i = 0; i < 2; i++) {
		buffer = &canvas->buffers[i];

		if (buffer->buffer != NULL) {
			wl_buffer_destroy(buffer->buffer);

			munmap(buffer->shm_data, buffer->shm_size);

			buffer->busy = 0;
			buffer->buffer = NULL;
		}
	}

	if (canvas->nextras > 0) {
		for (i = 0; i < canvas->nextras; i++) {
			buffer = &canvas->extras[i];

			if (buffer->buffer != NULL) {
				wl_buffer_destroy(buffer->buffer);

				munmap(buffer->shm_data, buffer->shm_size);

				buffer->busy = 0;
				buffer->buffer = NULL;
			}
		}

		free(canvas->extras);
	}

	if (canvas->nemo_surface != NULL)
		nemo_surface_destroy(canvas->nemo_surface);

	if (canvas->subsurface != NULL)
		wl_subsurface_destroy(canvas->subsurface);

	wl_surface_destroy(canvas->surface);

	pixman_region32_fini(&canvas->damage);

	nemolist_remove(&canvas->repaint_task.link);

	nemotool_unwatch_fd(canvas->tool, canvas->eventfd);
	close(canvas->eventfd);

	free(canvas);
}

pixman_image_t *nemocanvas_get_pixman_image(struct nemocanvas *canvas)
{
	pixman_image_t *image;

	if (canvas->buffer == NULL)
		return NULL;

	image = pixman_image_create_bits(PIXMAN_a8r8g8b8, canvas->width, canvas->height, (uint32_t *)canvas->buffer->shm_data, canvas->width * 4);

	return image;
}

void nemocanvas_move(struct nemocanvas *canvas, uint32_t serial)
{
	nemo_surface_move(canvas->nemo_surface, canvas->tool->seat, serial);
}

void nemocanvas_pick(struct nemocanvas *canvas, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	nemo_surface_pick(canvas->nemo_surface, canvas->tool->seat, serial0, serial1, type);
}

void nemocanvas_miss(struct nemocanvas *canvas)
{
	nemo_surface_miss(canvas->nemo_surface);
}

void nemocanvas_follow(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t degree, uint32_t delay, uint32_t duration)
{
	nemo_surface_follow(canvas->nemo_surface, x, y, degree, delay, duration);
}

void nemocanvas_execute(struct nemocanvas *canvas, const char *name, const char *cmds)
{
	nemo_surface_execute(canvas->nemo_surface, name, cmds);
}

void nemocanvas_set_size(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	canvas->width = width;
	canvas->height = height;

	if (canvas->nemo_surface != NULL)
		nemo_surface_set_size(canvas->nemo_surface, width, height);
}

void nemocanvas_set_min_size(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	if (canvas->nemo_surface != NULL)
		nemo_surface_set_min_size(canvas->nemo_surface, width, height);
}

void nemocanvas_set_max_size(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	if (canvas->nemo_surface != NULL)
		nemo_surface_set_max_size(canvas->nemo_surface, width, height);
}

void nemocanvas_set_scale(struct nemocanvas *canvas, float sx, float sy)
{
	if (canvas->nemo_surface != NULL)
		nemo_surface_set_scale(canvas->nemo_surface,
				wl_fixed_from_double(sx),
				wl_fixed_from_double(sy));
}

void nemocanvas_set_input_type(struct nemocanvas *canvas, uint32_t type)
{
	nemo_surface_set_input_type(canvas->nemo_surface, type);
}

void nemocanvas_set_input(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height)
{
	nemo_surface_set_input(canvas->nemo_surface, x, y, width, height);
}

void nemocanvas_set_pivot(struct nemocanvas *canvas, int px, int py)
{
	nemo_surface_set_pivot(canvas->nemo_surface, px, py);
}

void nemocanvas_set_anchor(struct nemocanvas *canvas, float ax, float ay)
{
	nemo_surface_set_anchor(canvas->nemo_surface,
			wl_fixed_from_double(ax),
			wl_fixed_from_double(ay));
}

void nemocanvas_set_flag(struct nemocanvas *canvas, float fx, float fy)
{
	nemo_surface_set_flag(canvas->nemo_surface,
			wl_fixed_from_double(fx),
			wl_fixed_from_double(fy));
}

void nemocanvas_set_layer(struct nemocanvas *canvas, uint32_t type)
{
	nemo_surface_set_layer(canvas->nemo_surface, type);
}

void nemocanvas_set_parent(struct nemocanvas *canvas, struct nemocanvas *parent)
{
	if (canvas->nemo_surface != NULL && parent->nemo_surface != NULL) {
		nemo_surface_set_parent(canvas->nemo_surface, parent->nemo_surface);
	}
}

void nemocanvas_set_fullscreen_type(struct nemocanvas *canvas, uint32_t type)
{
	nemo_surface_set_fullscreen_type(canvas->nemo_surface, type);
}

void nemocanvas_set_fullscreen_opaque(struct nemocanvas *canvas, uint32_t opaque)
{
	nemo_surface_set_fullscreen_opaque(canvas->nemo_surface, opaque);
}

void nemocanvas_set_fullscreen(struct nemocanvas *canvas, uint32_t id)
{
	nemo_surface_set_fullscreen(canvas->nemo_surface, id);
}

void nemocanvas_unset_fullscreen(struct nemocanvas *canvas)
{
	nemo_surface_unset_fullscreen(canvas->nemo_surface);
}

void nemocanvas_set_sound(struct nemocanvas *canvas)
{
	nemo_surface_set_sound(canvas->nemo_surface);
}

void nemocanvas_unset_sound(struct nemocanvas *canvas)
{
	nemo_surface_unset_sound(canvas->nemo_surface);
}

void nemocanvas_set_nemosurface(struct nemocanvas *canvas, uint32_t type)
{
	struct nemo_shell *shell = canvas->tool->shell;

	canvas->nemo_surface = nemo_shell_get_nemo_surface(shell, canvas->surface, type);
	nemo_surface_add_listener(canvas->nemo_surface, &nemo_surface_listener, canvas);
}

int nemocanvas_set_subsurface(struct nemocanvas *canvas, struct nemocanvas *parent)
{
	struct nemotool *tool = parent->tool;

	if (tool->subcompositor == NULL)
		return -1;

	if (canvas->nemo_surface != NULL) {
		nemo_surface_destroy(canvas->nemo_surface);
		canvas->nemo_surface = NULL;
	}

	canvas->subsurface = wl_subcompositor_get_subsurface(tool->subcompositor,
			canvas->surface,
			parent->surface);

	canvas->parent = parent;
}

void nemocanvas_set_subsurface_position(struct nemocanvas *canvas, int32_t x, int32_t y)
{
	wl_subsurface_set_position(canvas->subsurface, x, y);
}

void nemocanvas_set_subsurface_sync(struct nemocanvas *canvas)
{
	wl_subsurface_set_sync(canvas->subsurface);
}

void nemocanvas_set_subsurface_desync(struct nemocanvas *canvas)
{
	wl_subsurface_set_desync(canvas->subsurface);
}

void nemocanvas_set_dispatch_event(struct nemocanvas *canvas, nemocanvas_dispatch_event_t dispatch)
{
	canvas->dispatch_event = dispatch;
}

void nemocanvas_set_dispatch_resize(struct nemocanvas *canvas, nemocanvas_dispatch_resize_t dispatch)
{
	canvas->dispatch_resize = dispatch;
}

void nemocanvas_set_dispatch_transform(struct nemocanvas *canvas, nemocanvas_dispatch_transform_t dispatch)
{
	canvas->dispatch_transform = dispatch;
}

void nemocanvas_set_dispatch_fullscreen(struct nemocanvas *canvas, nemocanvas_dispatch_fullscreen_t dispatch)
{
	canvas->dispatch_fullscreen = dispatch;
}

void nemocanvas_set_framerate(struct nemocanvas *canvas, uint32_t framerate)
{
	canvas->framerate = framerate;
}

void nemocanvas_set_dispatch_frame(struct nemocanvas *canvas, nemocanvas_dispatch_frame_t dispatch)
{
	canvas->dispatch_frame = dispatch;
}

void nemocanvas_set_dispatch_screen(struct nemocanvas *canvas, nemocanvas_dispatch_screen_t dispatch)
{
	canvas->dispatch_screen = dispatch;
}

void nemocanvas_set_dispatch_destroy(struct nemocanvas *canvas, nemocanvas_dispatch_destroy_t dispatch)
{
	canvas->dispatch_destroy = dispatch;
}

static void presentation_feedback_sync_output(void *data, struct presentation_feedback *feedback, struct wl_output *output)
{
}

static void presentation_feedback_presented(void *data, struct presentation_feedback *feedback,
		uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	if (canvas->feedback != NULL) {
		presentation_feedback_destroy(canvas->feedback);

		canvas->feedback = NULL;
	}

	canvas->dispatch_frame(canvas, ((uint64_t)tv_sec_hi << 32) + tv_sec_lo, tv_nsec);
}

static void presentation_feedback_discarded(void *data, struct presentation_feedback *feedback)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	if (canvas->feedback != NULL) {
		presentation_feedback_destroy(canvas->feedback);

		canvas->feedback = NULL;
	}
}

static const struct presentation_feedback_listener presentation_feedback_listener = {
	presentation_feedback_sync_output,
	presentation_feedback_presented,
	presentation_feedback_discarded
};

void nemocanvas_feedback(struct nemocanvas *canvas)
{
	if (canvas->framerate == 0) {
		if (canvas->feedback != NULL) {
			presentation_feedback_destroy(canvas->feedback);
		}

		canvas->feedback = presentation_feedback(canvas->tool->presentation, canvas->surface);
		presentation_feedback_add_listener(canvas->feedback, &presentation_feedback_listener, canvas);
	} else {
		canvas->framefeed = 1;
		nemotimer_set_timeout(canvas->frametimer, 1000 / canvas->framerate);
	}
}

void nemocanvas_dispatch_frame(struct nemocanvas *canvas)
{
	if (canvas->framerate == 0) {
		if (canvas->feedback == NULL) {
			canvas->dispatch_frame(canvas, 0, 0);
		}
	} else {
		if (canvas->framefeed == 0) {
			canvas->dispatch_frame(canvas, 0, 0);
		}
	}
}

void nemocanvas_dispatch_frame_force(struct nemocanvas *canvas)
{
	canvas->dispatch_frame(canvas, 0, 0);
}

void nemocanvas_dispatch_frame_async(struct nemocanvas *canvas)
{
	uint64_t v = 1;

	write(canvas->eventfd, &v, sizeof(uint64_t));
}

void nemocanvas_dispatch_destroy(struct nemocanvas *canvas)
{
	if (canvas->dispatch_destroy != NULL)
		canvas->dispatch_destroy(canvas);
}

void nemocanvas_attach_queue(struct nemocanvas *canvas, struct nemoqueue *queue)
{
	if (canvas->surface != NULL)
		wl_proxy_set_queue((struct wl_proxy *)canvas->surface, queue->queue);
	if (canvas->nemo_surface != NULL)
		wl_proxy_set_queue((struct wl_proxy *)canvas->nemo_surface, queue->queue);
}

void nemocanvas_detach_queue(struct nemocanvas *canvas)
{
	if (canvas->surface != NULL)
		wl_proxy_set_queue((struct wl_proxy *)canvas->surface, NULL);
	if (canvas->nemo_surface != NULL)
		wl_proxy_set_queue((struct wl_proxy *)canvas->nemo_surface, NULL);
}
