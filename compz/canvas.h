#ifndef	__NEMO_CANVAS_H__
#define	__NEMO_CANVAS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>
#include <content.h>

struct nemocompz;
struct nemoscreen;

struct nemobuffer {
	struct wl_resource *resource;
	struct wl_signal destroy_signal;
	struct wl_listener destroy_listener;

	union {
		struct wl_shm_buffer *shmbuffer;
		void *legacy_buffer;
	};

	int32_t width, height;
	uint32_t busy_count;
	int y_inverted;
	int format;
};

struct nemobuffer_viewport {
	struct {
		uint32_t transform;

		int32_t scale;

		int viewport_set;

		double src_x, src_y;
		double src_width, src_height;
	} buffer;

	struct {
		int32_t width, height;
	} canvas;

	int changed;
};

struct nemobuffer_reference {
	struct nemobuffer *buffer;
	struct wl_listener destroy_listener;
};

struct nemoframe_callback {
	struct wl_resource *resource;
	struct wl_list link;
};

struct nemofeedback {
	struct wl_resource *resource;
	struct wl_list link;

	uint32_t psf_flags;
};

struct nemocanvas_state {
	int newly_attached;

	struct nemobuffer *buffer;
	struct nemobuffer_viewport buffer_viewport;
	struct wl_listener buffer_destroy_listener;
	int32_t sx, sy;

	pixman_region32_t damage;
	pixman_region32_t opaque;
	pixman_region32_t input;

	struct wl_list frame_callback_list;
	struct wl_list feedback_list;
};

struct nemocanvas {
	struct nemocontent base;

	struct nemocompz *compz;

	struct wl_resource *resource;
	struct wl_signal destroy_signal;
	struct wl_signal damage_signal;

	struct nemobuffer_reference buffer_reference;
	struct nemobuffer_viewport buffer_viewport;
	int32_t width_from_buffer;
	int32_t height_from_buffer;

	struct wl_resource *viewport_resource;

	struct wl_listener buffer_destroy_listener;

	struct nemocanvas_state pending;

	struct wl_list link;
	struct wl_list feedback_link;

	struct wl_list view_list;
	struct wl_list frame_callback_list;
	struct wl_list feedback_list;

	void (*configure)(struct nemocanvas *canvas, int32_t sx, int32_t sy);
	void *configure_private;

	void (*update_transform)(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height);
	void (*update_layer)(struct nemocanvas *canvas, int visible);
	void (*update_fullscreen)(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);

	struct wl_list subcanvas_list;

	uint64_t touchid0;

	uint32_t frame_count;
	uint64_t frame_damage;
};

struct nemocanvas_callback {
	void (*send_configure)(struct nemocanvas *canvas, int32_t width, int32_t height);
	void (*send_transform)(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height);
	void (*send_layer)(struct nemocanvas *canvas, int visible);
	void (*send_fullscreen)(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);
	void (*send_close)(struct nemocanvas *canvas);
};

extern struct nemocanvas *nemocanvas_create(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id);
extern void nemocanvas_destroy(struct nemocanvas *canvas);

extern void nemocanvas_size_from_buffer(struct nemocanvas *canvas);
extern void nemocanvas_set_size(struct nemocanvas *canvas, int32_t width, int32_t height);
extern void nemocanvas_update_size(struct nemocanvas *canvas);
extern void nemocanvas_schedule_repaint(struct nemocanvas *canvas);
extern void nemocanvas_commit_state(struct nemocanvas *canvas, struct nemocanvas_state *state);
extern void nemocanvas_commit(struct nemocanvas *canvas);
extern void nemocanvas_reset_pending_buffer(struct nemocanvas *canvas);

extern void nemocanvas_state_prepare(struct nemocanvas_state *state);
extern void nemocanvas_state_finish(struct nemocanvas_state *state);
extern void nemocanvas_state_set_buffer(struct nemocanvas_state *state, struct nemobuffer *buffer);
extern void nemocanvas_flush_damage(struct nemocanvas *canvas);

extern struct nemoview *nemocanvas_get_default_view(struct nemocanvas *canvas);

extern void nemobuffer_reference(struct nemobuffer_reference *ref, struct nemobuffer *buffer);

extern uint32_t nemocanvas_get_opengl_texture(struct nemocanvas *canvas, int index);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
