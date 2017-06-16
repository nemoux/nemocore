#ifndef	__NEMO_CONTENT_H__
#define	__NEMO_CONTENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>

#include <renderer.h>

struct nemopointer;
struct nemokeyboard;
struct nemokeypad;
struct touchpoint;
struct nemocontent;

typedef void (*nemocontent_key_handler_t)(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, void *data);
typedef void (*nemocontent_button_handler_t)(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, void *data);
typedef void (*nemocontent_touch_handler_t)(struct touchpoint *tp, struct nemocontent *content, uint32_t time, void *data);

struct nemocontent {
	pixman_region32_t damage;
	pixman_region32_t opaque;
	pixman_region32_t input;

	int32_t width, height;

	struct {
		pixman_region32_t input;
		int has_input;
	} region;

	uint32_t node_mask;
	uint32_t screen_mask;
	uint32_t screen_dirty;
	int dirty;

	nemocontent_key_handler_t key_handler;
	nemocontent_button_handler_t button_handler;
	nemocontent_touch_handler_t touch_handler;
	void *data;

	void **pcontexts;
	void **gcontexts;

	void (*get_viewport_transform)(struct nemocontent *content, pixman_transform_t *transform);
	int32_t (*get_buffer_scale)(struct nemocontent *content);
	void (*transform_to_buffer_point)(struct nemocontent *content, float sx, float sy, float *bx, float *by);
	pixman_box32_t (*transform_to_buffer_rect)(struct nemocontent *content, pixman_box32_t rect);

	int (*pick)(struct nemocontent *content, float x, float y);

	void (*update_output)(struct nemocontent *content);
	void (*update_transform)(struct nemocontent *content, int visible, int32_t x, int32_t y, int32_t width, int32_t height);
	void (*update_layer)(struct nemocontent *content, int visible);
	void (*update_fullscreen)(struct nemocontent *content, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);
	int (*read_pixels)(struct nemocontent *content, pixman_format_code_t format, void *pixels);

	void (*pointer_enter)(struct nemopointer *pointer, struct nemocontent *content);
	void (*pointer_leave)(struct nemopointer *pointer, struct nemocontent *content);
	void (*pointer_motion)(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, float x, float y);
	void (*pointer_axis)(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t axis, float value);
	void (*pointer_button)(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, uint32_t state);

	void (*keyboard_enter)(struct nemokeyboard *keyboard, struct nemocontent *content);
	void (*keyboard_leave)(struct nemokeyboard *keyboard, struct nemocontent *content);
	void (*keyboard_key)(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state);
	void (*keyboard_modifiers)(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

	void (*keypad_enter)(struct nemokeypad *keypad, struct nemocontent *content);
	void (*keypad_leave)(struct nemokeypad *keypad, struct nemocontent *content);
	void (*keypad_key)(struct nemokeypad *keypad, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state);
	void (*keypad_modifiers)(struct nemokeypad *keypad, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);

	void (*touch_down)(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy);
	void (*touch_up)(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid);
	void (*touch_motion)(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy);
	void (*touch_pressure)(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float p);
	void (*touch_frame)(struct touchpoint *tp, struct nemocontent *content);
};

extern int nemocontent_prepare(struct nemocontent *content, int nodemax);
extern void nemocontent_finish(struct nemocontent *content);

extern void nemocontent_get_viewport_transform(struct nemocontent *content, pixman_transform_t *transform);
extern int32_t nemocontent_get_buffer_scale(struct nemocontent *content);
extern void nemocontent_transform_to_buffer_point(struct nemocontent *content, float sx, float sy, float *bx, float *by);
extern pixman_box32_t nemocontent_transform_to_buffer_rect(struct nemocontent *content, pixman_box32_t rect);

extern void nemocontent_update_output(struct nemocontent *content);
extern void nemocontent_update_transform(struct nemocontent *content, int visible, int32_t x, int32_t y, int32_t width, int32_t height);
extern void nemocontent_update_layer(struct nemocontent *content, int visible);
extern void nemocontent_update_fullscreen(struct nemocontent *content, const char *id, int32_t x, int32_t y, int32_t width, int32_t height);

static inline void nemocontent_set_key_handler(struct nemocontent *content, nemocontent_key_handler_t handler, void *data)
{
	content->key_handler = handler;
	content->data = data;
}

static inline void nemocontent_set_button_handler(struct nemocontent *content, nemocontent_button_handler_t handler, void *data)
{
	content->button_handler = handler;
	content->data = data;
}

static inline void nemocontent_set_touch_handler(struct nemocontent *content, nemocontent_touch_handler_t handler, void *data)
{
	content->touch_handler = handler;
	content->data = data;
}

static inline int nemocontent_read_pixels(struct nemocontent *content, pixman_format_code_t format, void *pixels)
{
	if (content->read_pixels != NULL)
		content->read_pixels(content, format, pixels);

	return -1;
}

static inline void nemocontent_pointer_enter(struct nemopointer *pointer, struct nemocontent *content)
{
	if (content->pointer_enter != NULL)
		content->pointer_enter(pointer, content);
}

static inline void nemocontent_pointer_leave(struct nemopointer *pointer, struct nemocontent *content)
{
	if (content->pointer_leave != NULL)
		content->pointer_leave(pointer, content);
}

static inline void nemocontent_pointer_motion(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, float x, float y)
{
	if (content->pointer_motion != NULL)
		content->pointer_motion(pointer, content, time, x, y);
}

static inline void nemocontent_pointer_axis(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t axis, float value)
{
	if (content->pointer_axis != NULL)
		content->pointer_axis(pointer, content, time, axis, value);
}

static inline void nemocontent_pointer_button(struct nemopointer *pointer, struct nemocontent *content, uint32_t time, uint32_t button, uint32_t state)
{
	if (content->button_handler != NULL) {
		if (state == WL_POINTER_BUTTON_STATE_PRESSED)
			content->button_handler(pointer, content, time, button, content->data);
	} else if (content->pointer_button != NULL) {
		content->pointer_button(pointer, content, time, button, state);
	}
}

static inline void nemocontent_keyboard_enter(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	if (content->keyboard_enter != NULL)
		content->keyboard_enter(keyboard, content);
}

static inline void nemocontent_keyboard_leave(struct nemokeyboard *keyboard, struct nemocontent *content)
{
	if (content->keyboard_leave != NULL)
		content->keyboard_leave(keyboard, content);
}

static inline void nemocontent_keyboard_key(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	if (content->key_handler != NULL) {
		if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
			content->key_handler(keyboard, content, time, key, content->data);
	} else if (content->keyboard_key != NULL) {
		content->keyboard_key(keyboard, content, time, key, state);
	}
}

static inline void nemocontent_keyboard_modifiers(struct nemokeyboard *keyboard, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	if (content->keyboard_modifiers != NULL)
		content->keyboard_modifiers(keyboard, content, mods_depressed, mods_latched, mods_locked, group);
}

static inline void nemocontent_keypad_enter(struct nemokeypad *keypad, struct nemocontent *content)
{
	if (content->keypad_enter != NULL)
		content->keypad_enter(keypad, content);
}

static inline void nemocontent_keypad_leave(struct nemokeypad *keypad, struct nemocontent *content)
{
	if (content->keypad_leave != NULL)
		content->keypad_leave(keypad, content);
}

static inline void nemocontent_keypad_key(struct nemokeypad *keypad, struct nemocontent *content, uint32_t time, uint32_t key, uint32_t state)
{
	if (content->keypad_key != NULL)
		content->keypad_key(keypad, content, time, key, state);
}

static inline void nemocontent_keypad_modifiers(struct nemokeypad *keypad, struct nemocontent *content, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
	if (content->keypad_modifiers != NULL)
		content->keypad_modifiers(keypad, content, mods_depressed, mods_latched, mods_locked, group);
}

static inline void nemocontent_touch_down(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	if (content->touch_handler != NULL)
		content->touch_handler(tp, content, time, content->data);
	else if (content->touch_down != NULL)
		content->touch_down(tp, content, time, touchid, x, y, gx, gy);
}

static inline void nemocontent_touch_up(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid)
{
	if (content->touch_up != NULL)
		content->touch_up(tp, content, time, touchid);
}

static inline void nemocontent_touch_motion(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float x, float y, float gx, float gy)
{
	if (content->touch_motion != NULL)
		content->touch_motion(tp, content, time, touchid, x, y, gx, gy);
}

static inline void nemocontent_touch_pressure(struct touchpoint *tp, struct nemocontent *content, uint32_t time, uint64_t touchid, float p)
{
	if (content->touch_pressure != NULL)
		content->touch_pressure(tp, content, time, touchid, p);
}

static inline void nemocontent_touch_frame(struct touchpoint *tp, struct nemocontent *content)
{
	if (content->touch_frame != NULL)
		content->touch_frame(tp, content);
}

static inline void nemocontent_set_pixman_context(struct nemocontent *content, struct rendernode *node, void *context)
{
	content->pcontexts[node->id] = context;
}

static inline void *nemocontent_get_pixman_context(struct nemocontent *content, struct rendernode *node)
{
	return content->pcontexts[node->id];
}

static inline void nemocontent_set_opengl_context(struct nemocontent *content, struct rendernode *node, void *context)
{
	content->gcontexts[node->id] = context;
}

static inline void *nemocontent_get_opengl_context(struct nemocontent *content, struct rendernode *node)
{
	return content->gcontexts[node->id];
}

static inline void *nemocontent_get_opengl_context_on(struct nemocontent *content, int index)
{
	return content->gcontexts[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
