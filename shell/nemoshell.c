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
#include <screen.h>
#include <viewanimation.h>
#include <nemotoken.h>
#include <waylandhelper.h>
#include <nemomisc.h>

static void nemo_send_configure(struct nemocanvas *canvas, int32_t width, int32_t height)
{
	struct shellbin *bin = nemoshell_get_bin(canvas);
	int32_t fixed = bin->next_state.fullscreen || bin->next_state.maximized;

	nemo_surface_send_configure(bin->resource, width, height, fixed);
}

static struct nemoclient nemo_client = {
	nemo_send_configure
};

static void nemo_surface_destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static void nemo_surface_move(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_MOVABLE_FLAG) {
		nemoshell_move_canvas(bin->shell, bin, serial);
	}
}

static void nemo_surface_pick(struct wl_client *client, struct wl_resource *resource, struct wl_resource *seat_resource, uint32_t serial0, uint32_t serial1, uint32_t type)
{
	struct nemoseat *seat = (struct nemoseat *)wl_resource_get_user_data(seat_resource);
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->flags & NEMO_SHELL_SURFACE_PICKABLE_FLAG) {
		nemoshell_pick_canvas(bin->shell, bin, serial0, serial1, type);
	}
}

static void nemo_surface_miss(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (bin->grabbed > 0)
		wl_signal_emit(&bin->ungrab_signal, bin);
}

static void nemo_surface_follow(struct wl_client *client, struct wl_resource *resource, int32_t x, int32_t y, int32_t degree, uint32_t delay, uint32_t duration)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct shellbin *pbin = bin->parent;
	double px, py, pr;
	double fx, fy;
	double diag, radian, fradian, dradian;

	if (bin == NULL || pbin == NULL)
		return;

	px = pbin->view->geometry.x;
	py = pbin->view->geometry.y;
	pr = pbin->view->geometry.r;

	fx = x;
	fy = y;
	diag = sqrtf(fx * fx + fy * fy);
	radian = pr;
	if (fx == 0.0f)
		fx = 0.00000001f;
	fradian = atan(fy / fx);
	if (fx < 0.0f)
		fradian += M_PI;
	if (fx >= 0.0f && fy >= 0.0f)
		fradian += 2.0f * M_PI;
	if (diag != 0)
		radian += fradian;
	dradian = degree * M_PI / 180.0f;

	if (duration == 0) {
		nemoview_set_position(bin->view,
				px + diag * cos(radian) - 0 * sin(radian),
				py + diag * sin(radian) + 0 * cos(radian));
		nemoview_set_rotation(bin->view, dradian + pr);

		nemoview_schedule_repaint(bin->view);
	} else {
		struct viewanimation *animation;

		animation = viewanimation_create(bin->view, NEMOEASE_CUBIC_OUT_TYPE, delay, duration);
		animation->type = NEMO_VIEW_TRANSLATE_ANIMATION | NEMO_VIEW_ROTATE_ANIMATION;
		animation->translate.x = px + diag * cos(radian) - 0 * sin(radian);
		animation->translate.y = py + diag * sin(radian) + 0 * cos(radian);
		animation->rotate.r = dradian + pr;

		viewanimation_revoke(bin->shell->compz, bin->view);
		viewanimation_dispatch(bin->shell->compz, animation);
	}
}

static void nemo_surface_execute(struct wl_client *client, struct wl_resource *resource, const char *name, const char *cmds)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;
	struct nemotoken *token;
	pid_t pid;

	token = nemotoken_create(cmds, strlen(cmds));
	nemotoken_divide(token, ':');
	nemotoken_update(token);

	pid = wayland_execute_path(name, nemotoken_get_tokens(token), NULL);
	if (pid > 0) {
		struct clientstate *state;

		state = nemoshell_create_client_state(shell, pid);
		if (state != NULL) {
			nemoview_transform_to_global(bin->view,
					bin->canvas->base.width * bin->view->geometry.ax,
					bin->canvas->base.height * bin->view->geometry.ay,
					&state->x, &state->y);
			state->r = bin->view->geometry.r;
			state->dx = 0.5f;
			state->dy = 0.5f;
			state->flags = NEMO_SHELL_SURFACE_ALL_FLAGS;
		}
	}

	nemotoken_destroy(token);
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

static void nemo_surface_set_scale(struct wl_client *client, struct wl_resource *resource, wl_fixed_t sx, wl_fixed_t sy)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_scale(bin->view,
			wl_fixed_to_double(sx),
			wl_fixed_to_double(sy));
}

static void nemo_surface_set_input_type(struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (type == NEMO_SURFACE_INPUT_TYPE_NORMAL) {
		nemoview_set_input_type(bin->view, NEMO_VIEW_INPUT_NORMAL);
	} else if (type == NEMO_SURFACE_INPUT_TYPE_TOUCH) {
		nemoview_set_input_type(bin->view, NEMO_VIEW_INPUT_TOUCH);
	}
}

static void nemo_surface_set_input(struct wl_client *client, struct wl_resource *resource, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemocontent *content = &bin->canvas->base;

	pixman_region32_init_rect(&content->region.input, x, y, width, height);

	content->region.has_input = 1;
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

static void nemo_surface_set_layer(struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (type == NEMO_SURFACE_LAYER_TYPE_BACKGROUND) {
		bin->layer = &bin->shell->background_layer;

		nemoview_put_state(bin->view, NEMO_VIEW_CATCHABLE_STATE);
	} else if (type == NEMO_SURFACE_LAYER_TYPE_SERVICE) {
		bin->layer = &bin->shell->service_layer;
	} else if (type == NEMO_SURFACE_LAYER_TYPE_OVERLAY) {
		bin->layer = &bin->shell->overlay_layer;
	} else if (type == NEMO_SURFACE_LAYER_TYPE_UNDERLAY) {
		bin->layer = &bin->shell->underlay_layer;
	}
}

static void nemo_surface_set_parent(struct wl_client *client, struct wl_resource *resource, struct wl_resource *parent_resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct shellbin *pbin = (struct shellbin *)wl_resource_get_user_data(parent_resource);

	nemoshell_set_parent_bin(bin, pbin);
}

static void nemo_surface_set_fullscreen_type(struct wl_client *client, struct wl_resource *resource, uint32_t type)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	if (type & (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PICK)) {
		bin->on_pickscreen = 1;
	} else {
		bin->on_pickscreen = 0;
	}

	if (type & (1 << NEMO_SURFACE_FULLSCREEN_TYPE_PITCH)) {
		bin->on_pitchscreen = 1;
	} else {
		bin->on_pitchscreen = 0;
	}
#endif
}

static void nemo_surface_set_fullscreen(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
#ifdef NEMOUX_WITH_FULLSCREEN
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (bin->flags & NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG) {
		struct shellscreen *screen;

		screen = nemoshell_get_fullscreen(shell, id);
		if (screen != NULL) {
			if (bin->grabbed > 0)
				wl_signal_emit(&bin->ungrab_signal, bin);

			nemoshell_set_fullscreen_bin(shell, bin, screen);

			if (screen->focus == NEMO_SHELL_FULLSCREEN_ALL_FOCUS) {
				nemoseat_set_keyboard_focus(bin->shell->compz->seat, bin->view);
				nemoseat_set_pointer_focus(bin->shell->compz->seat, bin->view);
			}
		}
	}
#endif
}

static void nemo_surface_unset_fullscreen(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);
	struct nemoshell *shell = bin->shell;

	if (bin->flags & NEMO_SHELL_SURFACE_MAXIMIZABLE_FLAG) {
		nemoshell_put_fullscreen_bin(shell, bin);
	}
}

static void nemo_surface_set_sound(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_set_state(bin->view, NEMO_VIEW_SOUND_STATE);
}

static void nemo_surface_unset_sound(struct wl_client *client, struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	nemoview_put_state(bin->view, NEMO_VIEW_SOUND_STATE);
}

static const struct nemo_surface_interface nemo_surface_implementation = {
	nemo_surface_destroy,
	nemo_surface_move,
	nemo_surface_pick,
	nemo_surface_miss,
	nemo_surface_follow,
	nemo_surface_execute,
	nemo_surface_set_size,
	nemo_surface_set_min_size,
	nemo_surface_set_max_size,
	nemo_surface_set_scale,
	nemo_surface_set_input_type,
	nemo_surface_set_input,
	nemo_surface_set_pivot,
	nemo_surface_set_anchor,
	nemo_surface_set_layer,
	nemo_surface_set_parent,
	nemo_surface_set_fullscreen_type,
	nemo_surface_set_fullscreen,
	nemo_surface_unset_fullscreen,
	nemo_surface_set_sound,
	nemo_surface_unset_sound
};

static void nemoshell_unbind_nemo_surface(struct wl_resource *resource)
{
	struct shellbin *bin = (struct shellbin *)wl_resource_get_user_data(resource);

	bin->resource = NULL;
}

static void nemo_use_unstable_version(struct wl_client *client, struct wl_resource *resource, int32_t version)
{
	if (version > 1) {
		wl_resource_post_error(resource, 1, "nemo-shell: version not implemented yet");
		return;
	}
}

static void nemo_get_nemo_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource, uint32_t type)
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

	bin = nemoshell_create_bin(shell, canvas, &nemo_client);
	if (bin == NULL) {
		wl_resource_post_error(surface_resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"failed to create shell surface");
		return;
	}

	nemoview_put_state(bin->view, NEMO_VIEW_CATCHABLE_STATE);

	if (shell->default_layer != NULL)
		bin->layer = shell->default_layer;

	bin->owner = sc;

	bin->resource = wl_resource_create(client, &nemo_surface_interface, 1, id);
	if (bin->resource == NULL) {
		wl_resource_post_no_memory(surface_resource);
		nemoshell_destroy_bin(bin);
		return;
	}

	wl_resource_set_implementation(bin->resource, &nemo_surface_implementation, bin, nemoshell_unbind_nemo_surface);

	if (type == NEMO_SHELL_SURFACE_TYPE_NORMAL) {
		bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;

		nemoshell_use_client_state(shell, bin, client);
	} else if (type == NEMO_SHELL_SURFACE_TYPE_FOLLOW) {
		bin->type = NEMO_SHELL_SURFACE_NORMAL_TYPE;
	} else if (type == NEMO_SHELL_SURFACE_TYPE_OVERLAY) {
		bin->type = NEMO_SHELL_SURFACE_OVERLAY_TYPE;
		bin->view->transform.type = NEMO_VIEW_TRANSFORM_OVERLAY;
	}
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

#define	NEMO_SERVER_VERSION		1

	if (args[0].i != NEMO_SERVER_VERSION) {
		wl_resource_post_error(resource,
				WL_DISPLAY_ERROR_INVALID_OBJECT,
				"incompatible version, server is %d client wants %d", NEMO_SERVER_VERSION, args[0].i);
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
