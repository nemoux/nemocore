#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <wayland-server.h>

#include <keymap.h>
#include <nemomisc.h>

static int nemoxkb_make_keymap(struct nemoxkbinfo *xkbinfo)
{
	char *keymap_str;

	xkbinfo->shift_mod = xkb_map_mod_get_index(xkbinfo->keymap, XKB_MOD_NAME_SHIFT);
	xkbinfo->caps_mod = xkb_map_mod_get_index(xkbinfo->keymap, XKB_MOD_NAME_CAPS);
	xkbinfo->ctrl_mod = xkb_map_mod_get_index(xkbinfo->keymap, XKB_MOD_NAME_CTRL);
	xkbinfo->alt_mod = xkb_map_mod_get_index(xkbinfo->keymap, XKB_MOD_NAME_ALT);
	xkbinfo->mod2_mod = xkb_map_mod_get_index(xkbinfo->keymap, "Mod2");
	xkbinfo->mod3_mod = xkb_map_mod_get_index(xkbinfo->keymap, "Mod3");
	xkbinfo->super_mod = xkb_map_mod_get_index(xkbinfo->keymap, XKB_MOD_NAME_LOGO);
	xkbinfo->mod5_mod = xkb_map_mod_get_index(xkbinfo->keymap, "Mod5");
	xkbinfo->num_led = xkb_map_led_get_index(xkbinfo->keymap, XKB_LED_NAME_NUM);
	xkbinfo->caps_led = xkb_map_led_get_index(xkbinfo->keymap, XKB_LED_NAME_CAPS);
	xkbinfo->scroll_led = xkb_map_led_get_index(xkbinfo->keymap, XKB_LED_NAME_SCROLL);

	keymap_str = xkb_map_get_as_string(xkbinfo->keymap);
	if (keymap_str == NULL)
		return -1;

	xkbinfo->keymap_size = strlen(keymap_str) + 1;

	xkbinfo->keymap_fd = os_file_create_temp("%s/nemo-shared-XXXXXX", getenv("XDG_RUNTIME_DIR"));
	if (xkbinfo->keymap_fd < 0)
		goto err1;

	if (ftruncate(xkbinfo->keymap_fd, xkbinfo->keymap_size) < 0)
		goto err2;

	xkbinfo->keymap_area = (char *)mmap(NULL, xkbinfo->keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, xkbinfo->keymap_fd, 0);
	if (xkbinfo->keymap_area == MAP_FAILED)
		goto err2;

	strcpy(xkbinfo->keymap_area, keymap_str);

	free(keymap_str);

	return 0;

err2:
	close(xkbinfo->keymap_fd);

err1:
	free(keymap_str);

	return -1;
}

struct nemoxkb *nemoxkb_create(void)
{
	struct nemoxkb *xkb;

	xkb = (struct nemoxkb *)malloc(sizeof(struct nemoxkb));
	if (xkb == NULL)
		return NULL;
	memset(xkb, 0, sizeof(struct nemoxkb));

	xkb->context = xkb_context_new((enum xkb_context_flags)0);
	if (xkb->context == NULL)
		goto err1;

	xkb->names.rules = "evdev";
	xkb->names.model = "pc105";
	xkb->names.layout = "us";

	xkb->xkbinfo = (struct nemoxkbinfo *)malloc(sizeof(struct nemoxkbinfo));
	if (xkb->xkbinfo == NULL)
		goto err2;
	memset(xkb->xkbinfo, 0, sizeof(struct nemoxkbinfo));

	if (xkb->xkbinfo->keymap == NULL) {
		xkb->xkbinfo->keymap = xkb_map_new_from_names(xkb->context, &xkb->names, (enum xkb_keymap_compile_flags)0);
		if (xkb->xkbinfo->keymap == NULL)
			goto err3;

		if (nemoxkb_make_keymap(xkb->xkbinfo) < 0)
			goto err4;
	}

	xkb->state = xkb_state_new(xkb->xkbinfo->keymap);
	if (xkb->state == NULL)
		goto err4;

	wl_array_init(&xkb->keys);

	return xkb;

err4:
	xkb_keymap_unref(xkb->xkbinfo->keymap);

err3:
	free(xkb->xkbinfo);

err2:
	xkb_context_unref(xkb->context);

err1:
	free(xkb);

	return NULL;
}

void nemoxkb_destroy(struct nemoxkb *xkb)
{
	wl_array_release(&xkb->keys);

	xkb_state_unref(xkb->state);
	xkb_keymap_unref(xkb->xkbinfo->keymap);
	xkb_context_unref(xkb->context);

	free(xkb->xkbinfo);
	free(xkb);
}

int nemoxkb_reset(struct nemoxkb *xkb)
{
	struct xkb_state *state;

	state = xkb_state_new(xkb->xkbinfo->keymap);
	if (state == NULL)
		return -1;

	xkb_state_unref(xkb->state);

	xkb->state = state;

	return 0;
}

int nemoxkb_update_key(struct nemoxkb *xkb, uint32_t key, enum xkb_key_direction direction)
{
	uint32_t mods_depressed, mods_latched, mods_locked, mods_lookup, group;
	uint32_t leds = 0;
	int changed = 0;

	xkb_state_update_key(xkb->state, key + 8, direction);

	mods_depressed = xkb_state_serialize_mods(xkb->state, XKB_STATE_DEPRESSED);
	mods_latched = xkb_state_serialize_mods(xkb->state, XKB_STATE_LATCHED);
	mods_locked = xkb_state_serialize_mods(xkb->state, XKB_STATE_LOCKED);
	group = xkb_state_serialize_group(xkb->state, XKB_STATE_EFFECTIVE);

	if (mods_depressed != xkb->mods_depressed ||
			mods_latched != xkb->mods_latched ||
			mods_locked != xkb->mods_locked ||
			group != xkb->group)
		changed = 1;

	xkb->mods_depressed = mods_depressed;
	xkb->mods_latched = mods_latched;
	xkb->mods_locked = mods_locked;
	xkb->group = group;

	xkb->modifiers_state = 0;

	mods_lookup = mods_depressed | mods_latched;
	if (mods_lookup & (1 << xkb->xkbinfo->ctrl_mod))
		xkb->modifiers_state |= MODIFIER_CTRL;
	if (mods_lookup & (1 << xkb->xkbinfo->alt_mod))
		xkb->modifiers_state |= MODIFIER_ALT;
	if (mods_lookup & (1 << xkb->xkbinfo->super_mod))
		xkb->modifiers_state |= MODIFIER_SUPER;
	if (mods_lookup & (1 << xkb->xkbinfo->shift_mod))
		xkb->modifiers_state |= MODIFIER_SHIFT;

	if (xkb_state_led_index_is_active(xkb->state, xkb->xkbinfo->num_led))
		leds |= LED_NUM_LOCK;
	if (xkb_state_led_index_is_active(xkb->state, xkb->xkbinfo->caps_led))
		leds |= LED_CAPS_LOCK;
	if (xkb_state_led_index_is_active(xkb->state, xkb->xkbinfo->scroll_led))
		leds |= LED_SCROLL_LOCK;
	xkb->leds_state = leds;

	return changed;
}
