#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>
#include <wayland-server.h>

#include <keymap.h>
#include <oshelper.h>

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

	xkbinfo->keymap_fd = os_create_anonymous_file(xkbinfo->keymap_size);
	if (xkbinfo->keymap_fd < 0)
		return -1;

	xkbinfo->keymap_area = (char *)mmap(NULL, xkbinfo->keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED, xkbinfo->keymap_fd, 0);
	if (xkbinfo->keymap_area == MAP_FAILED)
		return -1;

	strcpy(xkbinfo->keymap_area, keymap_str);

	free(keymap_str);

	return 0;
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
		return NULL;

	xkb->names.rules = "evdev";
	xkb->names.model = "pc105";
	xkb->names.layout = "us";

	xkb->xkbinfo = (struct nemoxkbinfo *)malloc(sizeof(struct nemoxkbinfo));
	if (xkb->xkbinfo == NULL)
		return NULL;
	memset(xkb->xkbinfo, 0, sizeof(struct nemoxkbinfo));

	if (xkb->xkbinfo->keymap == NULL) {
		xkb->xkbinfo->keymap = xkb_map_new_from_names(xkb->context, &xkb->names, (enum xkb_keymap_compile_flags)0);
		if (xkb->xkbinfo->keymap == NULL)
			return NULL;

		if (nemoxkb_make_keymap(xkb->xkbinfo) < 0)
			return NULL;
	}

	xkb->state = xkb_state_new(xkb->xkbinfo->keymap);
	if (xkb->state == NULL)
		return NULL;

	return xkb;
}

void nemoxkb_destroy(struct nemoxkb *xkb)
{
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
