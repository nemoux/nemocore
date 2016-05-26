#ifndef	__NEMO_KEYMAP_H__
#define	__NEMO_KEYMAP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <xkbcommon/xkbcommon.h>

typedef enum {
	MODIFIER_CTRL = (1 << 0),
	MODIFIER_ALT = (1 << 1),
	MODIFIER_SUPER = (1 << 2),
	MODIFIER_SHIFT = (1 << 3)
} NemoXKBModifier;

typedef enum {
	LED_NUM_LOCK = (1 << 0),
	LED_CAPS_LOCK = (1 << 1),
	LED_SCROLL_LOCK = (1 << 2)
} NemoXKBLedLock;

struct nemoxkbinfo {
	struct xkb_keymap *keymap;
	int keymap_fd;
	size_t keymap_size;
	char *keymap_area;
	xkb_mod_index_t shift_mod;
	xkb_mod_index_t caps_mod;
	xkb_mod_index_t ctrl_mod;
	xkb_mod_index_t alt_mod;
	xkb_mod_index_t mod2_mod;
	xkb_mod_index_t mod3_mod;
	xkb_mod_index_t super_mod;
	xkb_mod_index_t mod5_mod;
	xkb_led_index_t num_led;
	xkb_led_index_t caps_led;
	xkb_led_index_t scroll_led;
};

struct nemoxkb {
	struct xkb_rule_names names;
	struct xkb_context *context;

	struct nemoxkbinfo *xkbinfo;

	struct xkb_state *state;

	uint32_t modifiers_state;
	uint32_t leds_state;

	uint32_t mods_depressed;
	uint32_t mods_latched;
	uint32_t mods_locked;
	uint32_t group;

	struct wl_array keys;
};

extern struct nemoxkb *nemoxkb_create(void);
extern void nemoxkb_destroy(struct nemoxkb *xkb);
extern int nemoxkb_reset(struct nemoxkb *xkb);

extern int nemoxkb_update_key(struct nemoxkb *xkb, uint32_t key, enum xkb_key_direction direction);

static inline int nemoxkb_has_modifiers_state(struct nemoxkb *xkb, uint32_t state)
{
	return xkb->modifiers_state == state;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
