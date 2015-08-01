#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-xdg-shell-client-protocol.h>

#include <nemotale.h>
#include <talegl.h>
#include <talepixman.h>
#include <nemolist.h>
#include <pixmanhelper.h>
#include <eglhelper.h>
#include <nemomisc.h>

struct taleapp {
	struct wl_display *display;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct xdg_shell *shell;

	EGLDisplay egl_display;
	EGLContext egl_context;
	EGLConfig egl_config;
	struct wl_egl_window *egl_window;

	int width, height;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct wl_callback *callback;
};

static void xdg_shell_ping(void *data, struct xdg_shell *shell, uint32_t serial)
{
	struct taleapp *app = (struct taleapp *)data;

	xdg_shell_pong(app->shell, serial);
}

static const struct xdg_shell_listener xdg_shell_listener = {
	xdg_shell_ping
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
	struct taleapp *app = (struct taleapp *)data;

	if (strcmp(interface, "wl_compositor") == 0) {
		app->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	} else if (strcmp(interface, "xdg_shell") == 0) {
		app->shell = wl_registry_bind(registry, id, &xdg_shell_interface, 1);
		xdg_shell_add_listener(app->shell, &xdg_shell_listener, app);
		xdg_shell_use_unstable_version(app->shell, 4);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static void xdg_surface_handle_configure(void *data, struct xdg_surface *surface, int32_t width, int32_t height, struct wl_array *states, uint32_t serial)
{
	struct taleapp *app = (struct taleapp *)data;

	app->width = width;
	app->height = height;

	wl_egl_window_resize(app->egl_window,
			app->width, app->height,
			0, 0);

	xdg_surface_ack_configure(surface, serial);
}

static void xdg_surface_handle_delete(void *data, struct xdg_surface *xdg_surface)
{
}

static const struct xdg_surface_listener xdg_surface_listener = {
	xdg_surface_handle_configure,
	xdg_surface_handle_delete
};

int main(int argc, char *argv[])
{
	struct taleapp *app;
	struct nemotale *tale0, *tale1;
	struct taleegl *egl;
	struct talefbo *fbo;
	struct talenode *node00, *node01, *node10, *node11;
	cairo_surface_t *surface;
	cairo_t *cr;

	app = (struct taleapp *)malloc(sizeof(struct taleapp));
	if (app == NULL)
		return -1;

	app->width = 320;
	app->height = 320;

	app->display = wl_display_connect(NULL);
	if (app->display == NULL)
		return -1;
	app->registry = wl_display_get_registry(app->display);
	wl_registry_add_listener(app->registry, &registry_listener, app);
	wl_display_dispatch(app->display);

	app->surface = wl_compositor_create_surface(app->compositor);
	app->xdg_surface = xdg_shell_get_xdg_surface(app->shell, app->surface);
	xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);
	xdg_surface_set_title(app->xdg_surface, "tale-fbo");

	egl_prepare_context((EGLNativeDisplayType)app->display, &app->egl_display, &app->egl_context, &app->egl_config, 0, NULL);
	egl_make_current(app->egl_display, app->egl_context);

	app->egl_window = wl_egl_window_create(app->surface, app->width, app->height);

	tale0 = nemotale_create_gl();
	egl = nemotale_create_egl(app->egl_display, app->egl_context, app->egl_config, (EGLNativeWindowType)app->egl_window);
	nemotale_set_backend(tale0, egl);
	nemotale_resize_gl(tale0, app->width, app->height);

	node00 = nemotale_node_create_pixman(80, 80);
	nemotale_attach_node(tale0, node00);

	surface = nemotale_node_get_cairo(node00);
	cr = cairo_create(surface);
	cairo_set_source_rgba(cr, 1.0f, 0.0f, 1.0f, 1.0f);
	cairo_paint(cr);
	cairo_destroy(cr);

	node01 = nemotale_node_create_gl(280, 280);
	nemotale_node_translate(node01, 40, 40);
	nemotale_attach_node(tale0, node01);

	tale1 = nemotale_create_gl();
	fbo = nemotale_create_fbo(nemotale_node_get_texture(node01), 280, 280);
	nemotale_set_backend(tale1, fbo);
	nemotale_resize_gl(tale1, 280, 280);

	node10 = nemotale_node_create_pixman(180, 180);
	nemotale_attach_node(tale1, node10);

	surface = nemotale_node_get_cairo(node10);
	cr = cairo_create(surface);
	cairo_set_source_rgba(cr, 1.0f, 1.0f, 0.0f, 1.0f);
	cairo_paint(cr);
	cairo_destroy(cr);

	node11 = nemotale_node_create_pixman(140, 140);
	nemotale_node_translate(node11, 140, 140);
	nemotale_attach_node(tale1, node11);

	surface = nemotale_node_get_cairo(node11);
	cr = cairo_create(surface);
	cairo_set_source_rgba(cr, 0.0f, 1.0f, 1.0f, 1.0f);
	cairo_paint(cr);
	cairo_destroy(cr);

	nemotale_composite_fbo(tale1, NULL);
	nemotale_composite_egl(tale0, NULL);

	while (1) {
		wl_display_dispatch(app->display);
	}

	nemotale_node_destroy(node00);
	nemotale_node_destroy(node01);
	nemotale_node_destroy(node10);
	nemotale_node_destroy(node11);
	nemotale_destroy_egl(egl);
	nemotale_destroy_fbo(fbo);
	nemotale_destroy_gl(tale1);
	nemotale_destroy_gl(tale0);

	wl_egl_window_destroy(app->egl_window);

	eglDestroyContext(app->egl_display, app->egl_context);

	xdg_surface_destroy(app->xdg_surface);
	wl_surface_destroy(app->surface);

	wl_compositor_destroy(app->compositor);
	wl_registry_destroy(app->registry);
	wl_display_flush(app->display);
	wl_display_disconnect(app->display);

	free(app);

	return 0;
}
