#ifndef	__NEMO_BINDING_H__
#define	__NEMO_BINDING_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <compz.h>

struct nemopointer;
struct nemokeyboard;
struct touchpoint;

typedef void (*nemobinding_key_handler_t)(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state, void *data);
typedef void (*nemobinding_button_handler_t)(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state, void *data);
typedef void (*nemobinding_touch_handler_t)(struct nemocompz *compz, struct touchpoint *tp, uint32_t time, void *data);

struct nemobinding {
	uint32_t key;
	uint32_t button;
	uint32_t axis;
	uint32_t modifier;

	void *handler;
	void *data;

	struct wl_list link;
};

extern struct nemobinding *nemobinding_create(uint32_t key, uint32_t button, uint32_t axis, uint32_t modifier, void *handler, void *data);
extern void nemobinding_destroy(struct nemobinding *binding);

extern struct nemobinding *nemocompz_add_key_binding(struct nemocompz *compz, uint32_t key, uint32_t modifier, nemobinding_key_handler_t handler, void *data);
extern void nemocompz_run_key_binding(struct nemocompz *compz, struct nemokeyboard *keyboard, uint32_t time, uint32_t key, enum wl_keyboard_key_state state);

extern struct nemobinding *nemocompz_add_button_binding(struct nemocompz *compz, uint32_t button, nemobinding_button_handler_t handler, void *data);
extern void nemocompz_run_button_binding(struct nemocompz *compz, struct nemopointer *pointer, uint32_t time, uint32_t button, enum wl_pointer_button_state state);

extern struct nemobinding *nemocompz_add_touch_binding(struct nemocompz *compz, nemobinding_touch_handler_t handler, void *data);
extern void nemocompz_run_touch_binding(struct nemocompz *compz, struct touchpoint *tp, uint32_t time);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
