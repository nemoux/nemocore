#ifndef	__NEMOTALE_H__
#define	__NEMOTALE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>
#include <cairo.h>

#include <nemolist.h>
#include <nemolistener.h>
#include <nemomatrix.h>
#include <nemobox.h>

typedef enum {
	NEMOTALE_NOFOCUS_STATE = (1 << 0)
} NemoTaleState;

struct nemotale;
struct talenode;
struct taleevent;

typedef void (*nemotale_dispatch_event_t)(struct nemotale *tale, struct talenode *node, struct taleevent *event);

struct nemotale {
	void *pmcontext;
	void *glcontext;
	void *backend;

	void *userdata;

	uint32_t state;

	struct talenode **nodes;
	int nnodes, snodes;

	struct nemosignal destroy_signal;

	int32_t width, height, stride;
	pixman_region32_t region;
	pixman_region32_t input;
	pixman_region32_t damage;

	pixman_format_code_t read_format;

	struct {
		struct nemomatrix matrix;

		int dirty;
	} transform;

	struct {
		int32_t width, height;

		double sx, sy;
		double rx, ry;

		int enable;
	} viewport;

	struct nemolist ptap_list;
	struct nemolist tap_list;
	struct nemolist grab_list;

	nemotale_dispatch_event_t dispatch_event;

	uint32_t long_press_duration;
	uint32_t long_press_distance;

	uint32_t single_click_duration;
	uint32_t single_click_distance;

	uint32_t close_width;
	uint32_t close_height;

	uint32_t minimum_width;
	uint32_t minimum_height;
	uint32_t maximum_width;
	uint32_t maximum_height;
};

#define	NEMOTALE_DESTROY_SIGNAL(tale)		(&tale->destroy_signal)
#define	NEMOTALE_DAMAGE(tale)						(&tale->damage)

extern int nemotale_prepare(struct nemotale *tale);
extern void nemotale_finish(struct nemotale *tale);

extern struct talenode *nemotale_pick_node(struct nemotale *tale, float x, float y, float *sx, float *sy);
extern int nemotale_contain_node(struct nemotale *tale, struct talenode *node, float x, float y, float *sx, float *sy);

extern void nemotale_damage_region(struct nemotale *tale, pixman_region32_t *region);
extern void nemotale_damage_below(struct nemotale *tale, struct talenode *node);
extern void nemotale_damage_all(struct nemotale *tale);
extern void nemotale_attach_node(struct nemotale *tale, struct talenode *node);
extern void nemotale_detach_node(struct talenode *node);
extern void nemotale_above_node(struct nemotale *tale, struct talenode *node, struct talenode *above);
extern void nemotale_below_node(struct nemotale *tale, struct talenode *node, struct talenode *below);

extern void nemotale_clear_node(struct nemotale *tale);

extern void nemotale_update_node(struct nemotale *tale);

extern void nemotale_accumulate_damage(struct nemotale *tale);
extern void nemotale_flush_damage(struct nemotale *tale);

static inline void nemotale_set_state(struct nemotale *tale, uint32_t state)
{
	tale->state |= state;
}

static inline void nemotale_put_state(struct nemotale *tale, uint32_t state)
{
	tale->state &= ~state;
}

static inline int nemotale_has_state(struct nemotale *tale, uint32_t state)
{
	return tale->state & state;
}

static inline void nemotale_resize(struct nemotale *tale, int32_t width, int32_t height)
{
	tale->width = width;
	tale->height = height;
	tale->transform.dirty = 1;

	pixman_region32_init_rect(&tale->region, 0, 0, width, height);
	pixman_region32_init_rect(&tale->input, 0, 0, width, height);

	if (tale->viewport.enable == 0) {
		tale->viewport.width = width;
		tale->viewport.height = height;
	} else {
		tale->viewport.sx = (double)tale->viewport.width / (double)tale->width;
		tale->viewport.sy = (double)tale->viewport.height / (double)tale->height;
		tale->viewport.rx = (double)tale->width / (double)tale->viewport.width;
		tale->viewport.ry = (double)tale->height / (double)tale->viewport.height;
	}
}

static inline void nemotale_set_width(struct nemotale *tale, int32_t width)
{
	tale->width = width;
	tale->transform.dirty = 1;

	pixman_region32_init_rect(&tale->region, 0, 0, tale->width, tale->height);
	pixman_region32_init_rect(&tale->input, 0, 0, tale->width, tale->height);

	if (tale->viewport.enable == 0) {
		tale->viewport.width = width;
	} else {
		tale->viewport.sx = (double)tale->viewport.width / (double)tale->width;
		tale->viewport.rx = (double)tale->width / (double)tale->viewport.width;
	}
}

static inline void nemotale_set_height(struct nemotale *tale, int32_t height)
{
	tale->height = height;
	tale->transform.dirty = 1;

	pixman_region32_init_rect(&tale->region, 0, 0, tale->width, tale->height);
	pixman_region32_init_rect(&tale->input, 0, 0, tale->width, tale->height);

	if (tale->viewport.enable == 0) {
		tale->viewport.height = height;
	} else {
		tale->viewport.sy = (double)tale->viewport.height / (double)tale->height;
		tale->viewport.ry = (double)tale->height / (double)tale->viewport.height;
	}
}

static inline void nemotale_set_viewport(struct nemotale *tale, int32_t width, int32_t height)
{
	tale->viewport.width = width;
	tale->viewport.height = height;

	tale->viewport.sx = (double)tale->viewport.width / (double)tale->width;
	tale->viewport.sy = (double)tale->viewport.height / (double)tale->height;
	tale->viewport.rx = (double)tale->width / (double)tale->viewport.width;
	tale->viewport.ry = (double)tale->height / (double)tale->viewport.height;

	tale->viewport.enable = 1;

	tale->transform.dirty = 1;
}

static inline void nemotale_transform(struct nemotale *tale, float d[9])
{
	nemomatrix_init_3x3(&tale->transform.matrix, d);

	tale->transform.dirty = 1;
}

static inline void nemotale_identity(struct nemotale *tale)
{
	nemomatrix_init_identity(&tale->transform.matrix);

	tale->transform.dirty = 1;
}

static inline void nemotale_translate(struct nemotale *tale, float x, float y)
{
	nemomatrix_translate(&tale->transform.matrix, x, y);

	tale->transform.dirty = 1;
}

static inline void nemotale_scale(struct nemotale *tale, float x, float y)
{
	nemomatrix_scale(&tale->transform.matrix, x, y);

	tale->transform.dirty = 1;
}

static inline void nemotale_rotate(struct nemotale *tale, float cos, float sin)
{
	nemomatrix_rotate(&tale->transform.matrix, cos, sin);

	tale->transform.dirty = 1;
}

static inline int nemotale_get_node_count(struct nemotale *tale)
{
	return tale->nnodes;
}

static inline int32_t nemotale_get_width(struct nemotale *tale)
{
	return tale->width;
}

static inline int32_t nemotale_get_height(struct nemotale *tale)
{
	return tale->height;
}

static inline void nemotale_set_dispatch_event(struct nemotale *tale, nemotale_dispatch_event_t dispatch)
{
	tale->dispatch_event = dispatch;
}

static inline int nemotale_has_dispatch_event(struct nemotale *tale)
{
	return tale->dispatch_event != NULL;
}

static inline void nemotale_set_long_press_gesture(struct nemotale *tale, uint32_t duration, uint32_t distance)
{
	tale->long_press_duration = duration;
	tale->long_press_distance = distance;
}

static inline void nemotale_set_single_click_gesture(struct nemotale *tale, uint32_t duration, uint32_t distance)
{
	tale->single_click_duration = duration;
	tale->single_click_distance = distance;
}

static inline void nemotale_set_close_width(struct nemotale *tale, uint32_t width)
{
	tale->close_width = width;
}

static inline void nemotale_set_close_height(struct nemotale *tale, uint32_t height)
{
	tale->close_height = height;
}

static inline void nemotale_set_minimum_width(struct nemotale *tale, uint32_t width)
{
	tale->minimum_width = width;
}

static inline void nemotale_set_minimum_height(struct nemotale *tale, uint32_t height)
{
	tale->minimum_height = height;
}

static inline void nemotale_set_maximum_width(struct nemotale *tale, uint32_t width)
{
	tale->maximum_width = width;
}

static inline void nemotale_set_maximum_height(struct nemotale *tale, uint32_t height)
{
	tale->maximum_height = height;
}

static inline uint32_t nemotale_get_close_width(struct nemotale *tale)
{
	return tale->close_width;
}

static inline uint32_t nemotale_get_close_height(struct nemotale *tale)
{
	return tale->close_height;
}

static inline uint32_t nemotale_get_minimum_width(struct nemotale *tale)
{
	return tale->minimum_width;
}

static inline uint32_t nemotale_get_minimum_height(struct nemotale *tale)
{
	return tale->minimum_height;
}

static inline uint32_t nemotale_get_maximum_width(struct nemotale *tale)
{
	return tale->maximum_width;
}

static inline uint32_t nemotale_get_maximum_height(struct nemotale *tale)
{
	return tale->maximum_height;
}

static inline void nemotale_set_backend(struct nemotale *tale, void *backend)
{
	tale->backend = backend;
}

static inline void *nemotale_get_backend(struct nemotale *tale)
{
	return tale->backend;
}

static inline void nemotale_set_userdata(struct nemotale *tale, void *data)
{
	tale->userdata = data;
}

static inline void *nemotale_get_userdata(struct nemotale *tale)
{
	return tale->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
