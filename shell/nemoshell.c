#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <assert.h>
#include <wayland-server.h>
#include <wayland-nemo-shell-server-protocol.h>

#include <compz.h>
#include <shell.h>
#include <nemoshell.h>
#include <canvas.h>
#include <subcanvas.h>
#include <view.h>
#include <move.h>
#include <pick.h>
#include <picker.h>
#include <screen.h>
#include <seat.h>
#include <viewanimation.h>
#include <nemomisc.h>

static void nemo_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);
	uint32_t serial = ++bin->next_serial;

	nemo_surface_send_configure(bin->resource, width, height, serial);
}

static void nemo_send_transform(struct nemocanvas *canvas, int visible, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_transform(bin->resource, visible, x, y, width, height);
}

static void nemo_send_layer(struct nemocanvas *canvas, int visible)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_layer(bin->resource, visible);
}

static void nemo_send_fullscreen(struct nemocanvas *canvas, const char *id, int32_t x, int32_t y, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_fullscreen(bin->resource, id, x, y, width, height);
}

static void nemo_send_close(struct nemocanvas *canvas)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);

	nemo_surface_send_close(bin->resource);
}

static struct nemocanvas_callback nemo_callback = {
	nemo_send_configure,
	nemo_send_transform,
	nemo_send_layer,
	nemo_send_fullscreen,
	nemo_send_close
};

static void nemo_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void nemo_surface_set_tag(struct wl_client *client, struct wl_resource *resource, uint32_t tag)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_tag(bin->view, tag);
}

static void nemo_surface_set_type(struct wl_client *client, struct wl_resource *resource, const char *type)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_type(bin->view, type);
}

static void nemo_surface_set_uuid(struct wl_client *client, struct wl_resource *resource, const char *uuid)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_uuid(bin->view, uuid);
}

static void nemo_surface_set_state(struct wl_client *client, struct wl_resource *resource, const char *state)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (strcmp(state, "pick") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_PICK_STATE);
	else if (strcmp(state, "keypad") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_KEYPAD_STATE);
	else if (strcmp(state, "sound") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_SOUND_STATE);
	else if (strcmp(state, "layer") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_LAYER_STATE);
	else if (strcmp(state, "opaque") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_OPAQUE_STATE);
	else if (strcmp(state, "stage") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_STAGE_STATE);
	else if (strcmp(state, "smooth") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_SMOOTH_STATE);
	else if (strcmp(state, "close") == 0)
		nemoview_set_state(bin->view, NEMOVIEW_CLOSE_STATE);
}

static void nemo_surface_put_state(struct wl_client *client, struct wl_resource *resource, const char *state)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (strcmp(state, "pick") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_PICK_STATE);
	else if (strcmp(state, "keypad") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_KEYPAD_STATE);
	else if (strcmp(state, "sound") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_SOUND_STATE);
	else if (strcmp(state, "layer") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_LAYER_STATE);
	else if (strcmp(state, "opaque") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_OPAQUE_STATE);
	else if (strcmp(state, "stage") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_STAGE_STATE);
	else if (strcmp(state, "smooth") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_SMOOTH_STATE);
	else if (strcmp(state, "close") == 0)
		nemoview_put_state(bin->view, NEMOVIEW_CLOSE_STATE);
}

static void nemo_surface_set_size(struct wl_client *client, struct wl_resource *resource, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->view->geometry.width = width;
	bin->view->geometry.height = height;
}

static void nemo_surface_set_min_size(struct wl_client *client, struct wl_resource *resource, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->min_width = width;
	bin->min_height = height;
}

static void nemo_surface_set_max_size(struct wl_client *client, struct wl_resource *resource, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->max_width = width;
	bin->max_height = height;
}

static void nemo_surface_set_position(struct wl_client *client, struct wl_resource *resource, wl_fixed_t x, wl_fixed_t y)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_position(bin->view,
			wl_fixed_to_double(x),
			wl_fixed_to_double(y));
}

static void nemo_surface_set_rotation(struct wl_client *client, struct wl_resource *resource, wl_fixed_t r)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_rotation(bin->view,
			wl_fixed_to_double(r));
}

static void nemo_surface_set_scale(struct wl_client *client, struct wl_resource *resource, wl_fixed_t sx, wl_fixed_t sy)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_scale(bin->view,
			wl_fixed_to_double(sx),
			wl_fixed_to_double(sy));
}

static void nemo_surface_set_pivot(struct wl_client *client, struct wl_resource *resource, int32_t px, int32_t py)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_pivot(bin->view, px, py);
}

static void nemo_surface_set_anchor(struct wl_client *client, struct wl_resource *resource, wl_fixed_t ax, wl_fixed_t ay)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_anchor(bin->view,
			wl_fixed_to_double(ax),
			wl_fixed_to_double(ay));
}

static void nemo_surface_set_flag(struct wl_client *client, struct wl_resource *resource, wl_fixed_t fx, wl_fixed_t fy)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_flag(bin->view,
			wl_fixed_to_double(fx),
			wl_fixed_to_double(fy));
}

static void nemo_surface_set_layer(struct wl_client *client, struct wl_resource *resource, const char *type)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	bin->layer = nemocompz_get_layer_by_name(shell->compz, type);
	if (bin->layer == NULL) {
		wl_resource_post_error(resource,
				NEMO_SURFACE_ERROR_INVALID_LAYER,
				"failed to attach shell surface to '%s' layer", type);
	}

	if (shell->update_layer != NULL)
		shell->update_layer(shell->userdata, bin, type);
}

static void nemo_surface_set_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent_resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct shellbin *pbin = (struct shellbin *)wl_resource_get_user_data(parent_resource);

	nemoshell_set_parent_bin(bin, pbin);
}

static void nemo_surface_set_region(struct wl_client *client, struct wl_resource *resource, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_region(bin->view, x, y, width, height);
}

static void nemo_surface_put_region(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_put_region(bin->view);
}

static void nemo_surface_set_scope(struct wl_client *client, struct wl_resource *resource, const char *cmds)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_scope(bin->view, cmds);
}

static void nemo_surface_put_scope(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_put_scope(bin->view);
}

static void nemo_surface_set_fullscreen_type(struct wl_client *client, struct wl_resource *resource, const char *type)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (strstr(type, "pick") != NULL)
		nemoshell_bin_set_state(bin, NEMOSHELL_BIN_PICKSCREEN_STATE);
	if (strstr(type, "pitch") != NULL)
		nemoshell_bin_set_state(bin, NEMOSHELL_BIN_PITCHSCREEN_STATE);
#endif
}

static void nemo_surface_put_fullscreen_type(struct wl_client *client, struct wl_resource *resource, const char *type)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (strstr(type, "pick") != NULL)
		nemoshell_bin_put_state(bin, NEMOSHELL_BIN_PICKSCREEN_STATE);
	if (strstr(type, "pitch") != NULL)
		nemoshell_bin_put_state(bin, NEMOSHELL_BIN_PITCHSCREEN_STATE);
#endif
}

static void nemo_surface_set_fullscreen_target(struct wl_client *client, struct wl_resource *resource, const char *id)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->fullscreen.target != NULL) {
		free(bin->fullscreen.target);

		bin->fullscreen.target = NULL;
	}

	if (id != NULL)
		bin->fullscreen.target = strdup(id);
}

static void nemo_surface_put_fullscreen_target(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->fullscreen.target != NULL) {
		free(bin->fullscreen.target);

		bin->fullscreen.target = NULL;
	}
}

static void nemo_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, const char *id)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG)) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(shell, id);
		if (screen != NULL) {
			if (nemoview_has_grab(bin->view) != 0)
				wl_signal_emit(&bin->ungrab_signal, bin);

			nemoshell_set_fullscreen_bin(shell, bin, screen);

			if (screen->focus == NEMOSHELL_FULLSCREEN_ALL_FOCUS) {
				nemoseat_set_keyboard_focus(shell->compz->seat, bin->view);
				nemoseat_set_pointer_focus(shell->compz->seat, bin->view);
			}
		}
	}
#endif
}

static void nemo_surface_put_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MAXIMIZABLE_FLAG))
		nemoshell_put_fullscreen_bin(shell, bin);
}

static void nemo_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_MOVABLE_FLAG)) {
		nemoshell_move_canvas(bin->shell, bin, serial);
	}
}

static void nemo_surface_pick(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial0, uint32_t serial1, const char *type)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (nemoshell_bin_has_flags(bin, NEMOSHELL_BIN_PICKABLE_FLAG)) {
		uint32_t ptype = 0x0;

		if (strstr(type, "rotate") != NULL)
			ptype |= NEMOSHELL_PICK_ROTATE_FLAG;
		if (strstr(type, "scale") != NULL)
			ptype |= NEMOSHELL_PICK_SCALE_FLAG;
		if (strstr(type, "translate") != NULL)
			ptype |= NEMOSHELL_PICK_TRANSLATE_FLAG;
		if (strstr(type, "resize") != NULL)
			ptype |= NEMOSHELL_PICK_RESIZE_FLAG;

		nemoshell_pick_canvas(bin->shell, bin, serial0, serial1, ptype);
	}
}

static void nemo_surface_miss(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (nemoview_has_grab(bin->view) != 0)
		wl_signal_emit(&bin->ungrab_signal, bin);
}

static void nemo_surface_focus_to(struct wl_client *client, struct wl_resource *resource, const char *uuid)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	nemoview_set_focus(bin->view, nemocompz_get_view_by_uuid(shell->compz, uuid));
}

static void nemo_surface_focus_on(struct wl_client *client, struct wl_resource *resource, wl_fixed_t x, wl_fixed_t y)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;
	float tx, ty;
	float sx, sy;

	nemoview_transform_to_global(bin->view,
			wl_fixed_to_double(x),
			wl_fixed_to_double(y),
			&tx, &ty);

	nemoview_set_focus(bin->view,
			nemocompz_pick_view(shell->compz, tx, ty, &sx, &sy, NEMOVIEW_PICK_STATE | NEMOVIEW_CANVAS_STATE | NEMOVIEW_KEYPAD_STATE));
}

static void nemo_surface_update(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->done_serial = serial;
}

static const struct nemo_surface_interface nemo_surface_implementation = {
	nemo_surface_destroy,
	nemo_surface_set_tag,
	nemo_surface_set_type,
	nemo_surface_set_uuid,
	nemo_surface_set_state,
	nemo_surface_put_state,
	nemo_surface_set_size,
	nemo_surface_set_min_size,
	nemo_surface_set_max_size,
	nemo_surface_set_position,
	nemo_surface_set_rotation,
	nemo_surface_set_scale,
	nemo_surface_set_pivot,
	nemo_surface_set_anchor,
	nemo_surface_set_flag,
	nemo_surface_set_layer,
	nemo_surface_set_parent,
	nemo_surface_set_region,
	nemo_surface_put_region,
	nemo_surface_set_scope,
	nemo_surface_put_scope,
	nemo_surface_set_fullscreen_type,
	nemo_surface_put_fullscreen_type,
	nemo_surface_set_fullscreen_target,
	nemo_surface_put_fullscreen_target,
	nemo_surface_set_fullscreen,
	nemo_surface_put_fullscreen,
	nemo_surface_move,
	nemo_surface_pick,
	nemo_surface_miss,
	nemo_surface_focus_to,
	nemo_surface_focus_on,
	nemo_surface_update
};

static void nemoshell_unbind_nemo_surface(struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->resource = NULL;
}

static void nemo_use_unstable_version(struct wl_client *client, struct wl_resource *resource, int32_t version)
{
	if (version > 1) {
		wl_resource_post_error(resource, 0, "nemo-shell: version not implemented yet");
		return;
	}
}

static void nemo_get_nemo_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, const char *type)
{
	struct nemocanvas *canvas = (struct nemocanvas *)wl_resource_get_user_data(surface_resource);
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = sc->shell;
	struct shellbin *bin;

	if (nemoshell_get_bin(canvas)) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"wayland shell surface is already registered");
		return;
	}

	bin = nemoshell_create_bin(shell, canvas, &nemo_callback);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	bin->owner = sc;

	wl_client_get_credentials(client, &bin->pid, NULL, NULL);

	bin->resource = wl_resource_create(client, &nemo_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &nemo_surface_implementation, bin, nemoshell_unbind_nemo_surface);

	if (strcmp(type, "normal") == 0) {
		bin->type = NEMOSHELL_BIN_NORMAL_TYPE;
	} else if (strcmp(type, "overlay") == 0) {
		bin->type = NEMOSHELL_BIN_OVERLAY_TYPE;
		bin->view->transform.type = NEMOVIEW_TRANSFORM_OVERLAY;
	}

	nemoshell_use_client_uuid(shell, bin);
}

static void nemo_pong(struct wl_client *client, struct wl_resource *resource, uint32_t serial)
{
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	nemoshell_pong(sc, serial);
}

static const struct nemo_shell_interface nemo_implementation = {
	nemo_use_unstable_version,
	nemo_get_nemo_surface,
	nemo_pong
};

static int nemoshell_dispatch(const void *implementation, void *target, uint32_t opcode, const struct wl_message *message, union wl_argument *args)
{
	struct wl_resource *resource = (struct wl_resource *)target;
	struct shellclient *sc = (struct shellclient *)wl_resource_get_user_data(resource);

	if (opcode != 0) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"must call use_unstable_version first");
		return 0;
	}

#define	NEMOSERVER_VERSION		1

	if (args[0].i != NEMOSERVER_VERSION) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"incompatible version, server is %d client wants %d", NEMOSERVER_VERSION, args[0].i);
		return 0;
	}

	wl_resource_set_implementation(resource, &nemo_implementation, sc, NULL);
}

int nemoshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemoshell *shell = (struct nemoshell *)data;
	struct shellclient *sc;

	sc = nemoshell_create_client(client, shell, &nemo_shell_interface, id);
	if (sc == NULL) {
		wl_client_post_no_memory(client);
		return -1;
	}

	wl_resource_set_dispatcher(sc->resource, nemoshell_dispatch, NULL, sc, NULL);

	return 0;
}

int nemoshell_is_nemo_surface(struct shellbin *bin)
{
	return bin && bin->resource && wl_resource_instance_of(bin->resource, &nemo_surface_interface, &nemo_surface_implementation);
}

int nemoshell_is_nemo_surface_for_canvas(struct nemocanvas *canvas)
{
	struct nemocanvas *parent;
	struct shellbin *pbin;

	if (canvas == NULL)
		return 0;

	parent = nemosubcanvas_get_main_canvas(canvas);
	if (parent == NULL)
		return 0;

	pbin = nemoshell_get_bin(parent);
	if (pbin != NULL && pbin->resource != NULL &&
			wl_resource_instance_of(pbin->resource, &nemo_surface_interface, &nemo_surface_implementation))
		return 1;

	return 0;
}
