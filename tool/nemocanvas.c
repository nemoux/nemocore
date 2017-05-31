#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <linux/input.h>

#include <pixman.h>

#include <wayland-client.h>
#include <wayland-nemo-seat-client-protocol.h>
#include <wayland-nemo-shell-client-protocol.h>
#include <wayland-presentation-timing-client-protocol.h>

#include <nemotool.h>
#include <nemocanvas.h>
#include <nemooutput.h>
#include <pixmanhelper.h>
#include <nemomisc.h>

static void nemo_surface_handle_configure(void *data, struct nemo_surface *surface, int32_t width, int32_t height, uint32_t serial)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	nemocanvas_update(canvas, serial);

	canvas->dispatch_resize(canvas, width, height);
}

static void nemo_surface_handle_transform(void *data, struct nemo_surface *surface, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	canvas->dispatch_transform(canvas, visible, x, y, width, height);
}

static void nemo_surface_handle_layer(void *data, struct nemo_surface *surface, int32_t visible)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	canvas->dispatch_layer(canvas, visible);
}

static void nemo_surface_handle_fullscreen(void *data, struct nemo_surface *surface, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	canvas->dispatch_fullscreen(canvas, id, x, y, width, height);
}

static void nemo_surface_handle_close(void *data, struct nemo_surface *surface)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	canvas->dispatch_destroy(canvas);
}

static const struct nemo_surface_listener nemo_surface_listener = {
	nemo_surface_handle_configure,
	nemo_surface_handle_transform,
	nemo_surface_handle_layer,
	nemo_surface_handle_fullscreen,
	nemo_surface_handle_close
};

static void buffer_release(void *data, struct wl_buffer *buffer)
{
	struct nemobuffer *nemobuffer = (struct nemobuffer *)data;

	nemobuffer->busy = 0;
}

static const struct wl_buffer_listener buffer_listener = {
	buffer_release
};

static void frame_handle_done(void *data, struct wl_callback *callback, uint32_t time)
{
	wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
	frame_handle_done
};

static int nemocanvas_get_shm_buffer(struct wl_shm *shm, struct nemobuffer *buffer, int width, int height, uint32_t format)
{
	struct wl_shm_pool *pool;
	int fd, size, stride;
	void *data;

	stride = width * 4;
	size = stride * height;

	fd = os_file_create_temp("%s/nemo-shared-XXXXXX", getenv("XDG_RUNTIME_DIR"));
	if (fd < 0)
		return -1;

	if (ftruncate(fd, size) < 0)
		goto err1;

	data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED)
		goto err1;

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

err1:
	close(fd);

	return -1;
}

int nemocanvas_ready(struct nemocanvas *canvas)
{
	struct nemobuffer *buffer = NULL;
	int i;

	for (i = 0; i < canvas->nbuffers; i++) {
		if (canvas->buffers[i].busy == 0) {
			buffer = &canvas->buffers[i];
			break;
		}
	}

	if (buffer == NULL)
		buffer = &canvas->buffers[0];

	if ((buffer->buffer != NULL) &&
			(buffer->width != canvas->width || buffer->height != canvas->height)) {
		wl_buffer_destroy(buffer->buffer);

		munmap(buffer->shm_data, buffer->shm_size);

		buffer->busy = 0;
		buffer->buffer = NULL;
	}

	if (buffer->buffer == NULL) {
		if (nemocanvas_get_shm_buffer(canvas->tool->shm, buffer, canvas->width, canvas->height, WL_SHM_FORMAT_ARGB8888) < 0)
			goto failed;

		memset(buffer->shm_data, 0xff, canvas->width * canvas->height * 4);
	}

	canvas->buffer = buffer;

	return 0;

failed:
	canvas->buffer = NULL;

	return -1;
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
	pixman_box32_t *boxes;
	int nboxes, i;

	wl_surface_attach(canvas->surface, canvas->buffer->buffer, 0, 0);

	boxes = pixman_region32_rectangles(&canvas->damage, &nboxes);

	for (i = 0; i < nboxes; i++) {
		wl_surface_damage(canvas->surface,
				boxes[i].x1,
				boxes[i].y1,
				boxes[i].x2 - boxes[i].x1,
				boxes[i].y2 - boxes[i].y1);
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

	output = nemooutput_find(canvas->tool, _output);
	if (output == NULL)
		return;

	canvas->dispatch_screen(canvas, output->x, output->y, output->width, output->height, output->mmwidth, output->mmheight, 0);
}

static void surface_leave(void *data, struct wl_surface *surface, struct wl_output *_output)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;
	struct nemooutput *output;

	output = nemooutput_find(canvas->tool, _output);
	if (output == NULL)
		return;

	canvas->dispatch_screen(canvas, output->x, output->y, output->width, output->height, output->mmwidth, output->mmheight, 1);
}

static const struct wl_surface_listener surface_listener = {
	surface_enter,
	surface_leave
};

static int nemocanvas_dispatch_event_none(struct nemocanvas *canvas, uint32_t type, struct nemoevent *event)
{
	return 0;
}

static void nemocanvas_dispatch_resize_none(struct nemocanvas *canvas, int32_t width, int32_t height)
{
}

static void nemocanvas_dispatch_transform_none(struct nemocanvas *canvas, int32_t visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void nemocanvas_dispatch_layer_none(struct nemocanvas *canvas, int32_t visible)
{
}

static void nemocanvas_dispatch_fullscreen_none(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
}

static void nemocanvas_dispatch_frame_none(struct nemocanvas *canvas, uint64_t secs, uint32_t nsecs)
{
}

static void nemocanvas_dispatch_discard_none(struct nemocanvas *canvas)
{
}

static void nemocanvas_dispatch_screen_none(struct nemocanvas *canvas, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mmwidth, int32_t mmheight, int left)
{
}

static int nemocanvas_dispatch_destroy_none(struct nemocanvas *canvas)
{
	return 0;
}

struct nemocanvas *nemocanvas_create(struct nemotool *tool)
{
	struct nemocanvas *canvas;

	canvas = (struct nemocanvas *)malloc(sizeof(struct nemocanvas));
	if (canvas == NULL)
		return NULL;
	memset(canvas, 0, sizeof(struct nemocanvas));

	canvas->buffers = (struct nemobuffer *)malloc(sizeof(struct nemobuffer[2]));
	if (canvas->buffers == NULL)
		goto err1;
	memset(canvas->buffers, 0, sizeof(struct nemobuffer[2]));

	canvas->nbuffers = 2;

	nemosignal_init(&canvas->destroy_signal);

	pixman_region32_init(&canvas->damage);

	canvas->tool = tool;

	canvas->framerate = 0;
	canvas->framemsecs = 0;

	canvas->surface = wl_compositor_create_surface(tool->compositor);
	wl_surface_add_listener(canvas->surface, &surface_listener, canvas);
	wl_surface_set_user_data(canvas->surface, canvas);

	canvas->dispatch_event = nemocanvas_dispatch_event_none;
	canvas->dispatch_resize = nemocanvas_dispatch_resize_none;
	canvas->dispatch_transform = nemocanvas_dispatch_transform_none;
	canvas->dispatch_layer = nemocanvas_dispatch_layer_none;
	canvas->dispatch_fullscreen = nemocanvas_dispatch_fullscreen_none;
	canvas->dispatch_frame = nemocanvas_dispatch_frame_none;
	canvas->dispatch_discard = nemocanvas_dispatch_discard_none;
	canvas->dispatch_screen = nemocanvas_dispatch_screen_none;
	canvas->dispatch_destroy = nemocanvas_dispatch_destroy_none;

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

	for (i = 0; i < canvas->nbuffers; i++) {
		buffer = &canvas->buffers[i];

		if (buffer->buffer != NULL) {
			wl_buffer_destroy(buffer->buffer);

			munmap(buffer->shm_data, buffer->shm_size);

			buffer->busy = 0;
			buffer->buffer = NULL;
		}
	}

	if (canvas->nemo_surface != NULL)
		nemo_surface_destroy(canvas->nemo_surface);

	if (canvas->subsurface != NULL)
		wl_subsurface_destroy(canvas->subsurface);

	wl_surface_destroy(canvas->surface);

	pixman_region32_fini(&canvas->damage);

	free(canvas->buffers);
	free(canvas);
}

int nemocanvas_init(struct nemocanvas *canvas, struct nemotool *tool)
{
	canvas->buffers = (struct nemobuffer *)malloc(sizeof(struct nemobuffer[2]));
	if (canvas->buffers == NULL)
		return -1;
	memset(canvas->buffers, 0, sizeof(struct nemobuffer[2]));

	canvas->nbuffers = 2;

	nemosignal_init(&canvas->destroy_signal);

	pixman_region32_init(&canvas->damage);

	canvas->tool = tool;

	canvas->framerate = 0;
	canvas->framemsecs = 0;

	canvas->surface = wl_compositor_create_surface(tool->compositor);
	wl_surface_add_listener(canvas->surface, &surface_listener, canvas);
	wl_surface_set_user_data(canvas->surface, canvas);

	canvas->dispatch_event = nemocanvas_dispatch_event_none;
	canvas->dispatch_resize = nemocanvas_dispatch_resize_none;
	canvas->dispatch_transform = nemocanvas_dispatch_transform_none;
	canvas->dispatch_layer = nemocanvas_dispatch_layer_none;
	canvas->dispatch_fullscreen = nemocanvas_dispatch_fullscreen_none;
	canvas->dispatch_frame = nemocanvas_dispatch_frame_none;
	canvas->dispatch_discard = nemocanvas_dispatch_discard_none;
	canvas->dispatch_screen = nemocanvas_dispatch_screen_none;
	canvas->dispatch_destroy = nemocanvas_dispatch_destroy_none;

	return 0;
}

void nemocanvas_exit(struct nemocanvas *canvas)
{
	struct nemobuffer *buffer;
	int i;

	nemosignal_emit(&canvas->destroy_signal, canvas);

	for (i = 0; i < canvas->nbuffers; i++) {
		buffer = &canvas->buffers[i];

		if (buffer->buffer != NULL) {
			wl_buffer_destroy(buffer->buffer);

			munmap(buffer->shm_data, buffer->shm_size);

			buffer->busy = 0;
			buffer->buffer = NULL;
		}
	}

	if (canvas->nemo_surface != NULL)
		nemo_surface_destroy(canvas->nemo_surface);

	if (canvas->subsurface != NULL)
		wl_subsurface_destroy(canvas->subsurface);

	wl_surface_destroy(canvas->surface);

	pixman_region32_fini(&canvas->damage);

	free(canvas->buffers);
}

pixman_image_t *nemocanvas_get_pixman_image(struct nemocanvas *canvas)
{
	pixman_image_t *image;

	if (canvas->buffer == NULL)
		return NULL;

	image = pixman_image_create_bits(PIXMAN_a8r8g8b8, canvas->width, canvas->height, (uint32_t *)canvas->buffer->shm_data, canvas->width * 4);

	return image;
}

int nemocanvas_set_buffer_max(struct nemocanvas *canvas, int nbuffers)
{
	canvas->buffers = (struct nemobuffer *)realloc(canvas->buffers, sizeof(struct nemobuffer[nbuffers]));
	if (canvas->buffers == NULL)
		return -1;
	memset(canvas->buffers, 0, sizeof(struct nemobuffer[nbuffers]));

	canvas->nbuffers = nbuffers;

	return 0;
}

void nemocanvas_set_tag(struct nemocanvas *canvas, uint32_t tag)
{
	nemo_surface_set_tag(canvas->nemo_surface, tag);
}

void nemocanvas_set_type(struct nemocanvas *canvas, const char *type)
{
	nemo_surface_set_type(canvas->nemo_surface, type);
}

void nemocanvas_set_uuid(struct nemocanvas *canvas, const char *uuid)
{
	nemo_surface_set_uuid(canvas->nemo_surface, uuid);
}

void nemocanvas_set_state(struct nemocanvas *canvas, const char *state)
{
	nemo_surface_set_state(canvas->nemo_surface, state);
}

void nemocanvas_put_state(struct nemocanvas *canvas, const char *state)
{
	nemo_surface_put_state(canvas->nemo_surface, state);
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

void nemocanvas_set_position(struct nemocanvas *canvas, float x, float y)
{
	if (canvas->nemo_surface != NULL)
		nemo_surface_set_position(canvas->nemo_surface,
				wl_fixed_from_double(x),
				wl_fixed_from_double(y));
}

void nemocanvas_set_rotation(struct nemocanvas *canvas, float r)
{
	if (canvas->nemo_surface != NULL)
		nemo_surface_set_rotation(canvas->nemo_surface,
				wl_fixed_from_double(r));
}

void nemocanvas_set_scale(struct nemocanvas *canvas, float sx, float sy)
{
	if (canvas->nemo_surface != NULL)
		nemo_surface_set_scale(canvas->nemo_surface,
				wl_fixed_from_double(sx),
				wl_fixed_from_double(sy));
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

void nemocanvas_set_layer(struct nemocanvas *canvas, const char *type)
{
	nemo_surface_set_layer(canvas->nemo_surface, type);
}

void nemocanvas_set_parent(struct nemocanvas *canvas, struct nemocanvas *parent)
{
	if (canvas->nemo_surface != NULL && parent->nemo_surface != NULL) {
		nemo_surface_set_parent(canvas->nemo_surface, parent->nemo_surface);
	}
}

void nemocanvas_set_region(struct nemocanvas *canvas, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	nemo_surface_set_region(canvas->nemo_surface, x, y, width, height);
}

void nemocanvas_put_region(struct nemocanvas *canvas)
{
	nemo_surface_put_region(canvas->nemo_surface);
}

void nemocanvas_set_scope(struct nemocanvas *canvas, const char *cmds)
{
	nemo_surface_set_scope(canvas->nemo_surface, cmds);
}

void nemocanvas_put_scope(struct nemocanvas *canvas)
{
	nemo_surface_put_scope(canvas->nemo_surface);
}

void nemocanvas_set_fullscreen_type(struct nemocanvas *canvas, const char *type)
{
	nemo_surface_set_fullscreen_type(canvas->nemo_surface, type);
}

void nemocanvas_put_fullscreen_type(struct nemocanvas *canvas, const char *type)
{
	nemo_surface_put_fullscreen_type(canvas->nemo_surface, type);
}

void nemocanvas_set_fullscreen_target(struct nemocanvas *canvas, const char *id)
{
	nemo_surface_set_fullscreen_target(canvas->nemo_surface, id);
}

void nemocanvas_put_fullscreen_target(struct nemocanvas *canvas)
{
	nemo_surface_put_fullscreen_target(canvas->nemo_surface);
}

void nemocanvas_set_fullscreen(struct nemocanvas *canvas, const char *id)
{
	nemo_surface_set_fullscreen(canvas->nemo_surface, id);
}

void nemocanvas_put_fullscreen(struct nemocanvas *canvas)
{
	nemo_surface_put_fullscreen(canvas->nemo_surface);
}

void nemocanvas_move(struct nemocanvas *canvas, uint32_t serial)
{
	nemo_surface_move(canvas->nemo_surface, canvas->tool->seat, serial);
}

void nemocanvas_pick(struct nemocanvas *canvas, uint32_t serial0, uint32_t serial1, const char *type)
{
	nemo_surface_pick(canvas->nemo_surface, canvas->tool->seat, serial0, serial1, type);
}

void nemocanvas_miss(struct nemocanvas *canvas)
{
	nemo_surface_miss(canvas->nemo_surface);
}

void nemocanvas_focus_to(struct nemocanvas *canvas, const char *uuid)
{
	nemo_surface_focus_to(canvas->nemo_surface, uuid);
}

void nemocanvas_focus_on(struct nemocanvas *canvas, double x, double y)
{
	nemo_surface_focus_on(canvas->nemo_surface,
			wl_fixed_from_double(x),
			wl_fixed_from_double(y));
}

void nemocanvas_update(struct nemocanvas *canvas, uint32_t serial)
{
	nemo_surface_update(canvas->nemo_surface, serial);
}

void nemocanvas_set_nemosurface(struct nemocanvas *canvas, const char *type)
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
	if (dispatch != NULL)
		canvas->dispatch_event = dispatch;
	else
		canvas->dispatch_event = nemocanvas_dispatch_event_none;
}

void nemocanvas_set_dispatch_resize(struct nemocanvas *canvas, nemocanvas_dispatch_resize_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_resize = dispatch;
	else
		canvas->dispatch_resize = nemocanvas_dispatch_resize_none;
}

void nemocanvas_set_dispatch_transform(struct nemocanvas *canvas, nemocanvas_dispatch_transform_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_transform = dispatch;
	else
		canvas->dispatch_transform = nemocanvas_dispatch_transform_none;
}

void nemocanvas_set_dispatch_layer(struct nemocanvas *canvas, nemocanvas_dispatch_layer_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_layer = dispatch;
	else
		canvas->dispatch_layer = nemocanvas_dispatch_layer_none;
}

void nemocanvas_set_dispatch_fullscreen(struct nemocanvas *canvas, nemocanvas_dispatch_fullscreen_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_fullscreen = dispatch;
	else
		canvas->dispatch_fullscreen = nemocanvas_dispatch_fullscreen_none;
}

void nemocanvas_set_framerate(struct nemocanvas *canvas, uint32_t framerate)
{
	canvas->framerate = framerate;
}

void nemocanvas_set_dispatch_frame(struct nemocanvas *canvas, nemocanvas_dispatch_frame_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_frame = dispatch;
	else
		canvas->dispatch_frame = nemocanvas_dispatch_frame_none;
}

void nemocanvas_set_dispatch_discard(struct nemocanvas *canvas, nemocanvas_dispatch_discard_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_discard = dispatch;
	else
		canvas->dispatch_discard = nemocanvas_dispatch_discard_none;
}

void nemocanvas_set_dispatch_screen(struct nemocanvas *canvas, nemocanvas_dispatch_screen_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_screen = dispatch;
	else
		canvas->dispatch_screen = nemocanvas_dispatch_screen_none;
}

void nemocanvas_set_dispatch_destroy(struct nemocanvas *canvas, nemocanvas_dispatch_destroy_t dispatch)
{
	if (dispatch != NULL)
		canvas->dispatch_destroy = dispatch;
	else
		canvas->dispatch_destroy = nemocanvas_dispatch_destroy_none;
}

static const struct presentation_feedback_listener presentation_feedback_listener;
static const struct presentation_feedback_listener presentation_feedback_framerate_listener;

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

static void presentation_feedback_framerate_presented(void *data, struct presentation_feedback *feedback,
		uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh,
		uint32_t seq_hi, uint32_t seq_lo, uint32_t flags)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;
	uint32_t msecs = (((uint64_t)tv_sec_hi << 32) + tv_sec_lo) * 1000 + tv_nsec / 1000000;

	if (canvas->feedback != NULL) {
		presentation_feedback_destroy(canvas->feedback);

		canvas->feedback = NULL;
	}

	if (canvas->framemsecs + (1000 / canvas->framerate) <= msecs) {
		canvas->dispatch_frame(canvas, ((uint64_t)tv_sec_hi << 32) + tv_sec_lo, tv_nsec);

		canvas->framemsecs = msecs;
	} else {
		canvas->feedback = presentation_feedback(canvas->tool->presentation, canvas->surface);
		presentation_feedback_add_listener(canvas->feedback, &presentation_feedback_framerate_listener, canvas);

		wl_surface_commit(canvas->surface);
	}
}

static void presentation_feedback_discarded(void *data, struct presentation_feedback *feedback)
{
	struct nemocanvas *canvas = (struct nemocanvas *)data;

	if (canvas->feedback != NULL) {
		presentation_feedback_destroy(canvas->feedback);

		canvas->feedback = NULL;
	}

	canvas->dispatch_discard(canvas);
}

static const struct presentation_feedback_listener presentation_feedback_listener = {
	presentation_feedback_sync_output,
	presentation_feedback_presented,
	presentation_feedback_discarded
};

static const struct presentation_feedback_listener presentation_feedback_framerate_listener = {
	presentation_feedback_sync_output,
	presentation_feedback_framerate_presented,
	presentation_feedback_discarded
};

void nemocanvas_dispatch_feedback(struct nemocanvas *canvas)
{
	if (canvas->feedback == NULL) {
		if (canvas->framerate == 0) {
			canvas->feedback = presentation_feedback(canvas->tool->presentation, canvas->surface);
			presentation_feedback_add_listener(canvas->feedback, &presentation_feedback_listener, canvas);
		} else {
			canvas->feedback = presentation_feedback(canvas->tool->presentation, canvas->surface);
			presentation_feedback_add_listener(canvas->feedback, &presentation_feedback_framerate_listener, canvas);
		}
	}
}

void nemocanvas_terminate_feedback(struct nemocanvas *canvas)
{
	if (canvas->feedback != NULL) {
		presentation_feedback_destroy(canvas->feedback);

		canvas->feedback = NULL;
	}
}

void nemocanvas_dispatch_frame(struct nemocanvas *canvas)
{
	if (canvas->feedback == NULL)
		canvas->dispatch_frame(canvas, 0, 0);
}

void nemocanvas_dispatch_resize(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	canvas->dispatch_resize(canvas, width, height);
}

int nemocanvas_dispatch_destroy(struct nemocanvas *canvas)
{
	return canvas->dispatch_destroy(canvas);
}
