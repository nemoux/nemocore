#ifndef	__NEMO_KEYMAP_H__
#define	__NEMO_KEYMAP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <xkbcommon/xkbcommon.h>

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
};

extern struct nemoxkb *nemoxkb_create(void);
extern void nemoxkb_destroy(struct nemoxkb *xkb);
extern int nemoxkb_reset(struct nemoxkb *xkb);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
