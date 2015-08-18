#ifndef	__WAYLAND_NODE_H__
#define	__WAYLAND_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <compz.h>
#include <backend.h>
#include <screen.h>
#include <renderer.h>

struct nemopointer;
struct nemokeyboard;
struct nemotouch;

struct waylandscreen {
	struct nemoscreen base;

	struct waylandnode *node;

	struct {
		struct wl_surface *surface;
		struct wl_shell_surface *shell_surface;
#ifdef NEMOUX_WITH_EGL
		struct wl_egl_window *egl_window;
#endif
		struct wl_output *output;
	} parent;

	struct nemomode mode;

	struct {
		struct wl_list buffers;
		struct wl_list free_buffers;
	} shm;

	int initial_frame;
};

struct waylandnode {
	struct rendernode base;

	struct {
		struct wl_display *display;
		struct wl_registry *registry;
		struct wl_compositor *compositor;
		struct wl_shell *shell;
		struct wl_shm *shm;

		struct wl_seat *seat;
		struct wl_pointer *pointer;
		struct wl_keyboard *keyboard;
		struct wl_touch *touch;

		uint32_t pointer_serial;
		uint32_t keyboard_serial;
	} parent;

	struct nemopointer *pointer;
	struct nemokeyboard *keyboard;
	struct nemotouch *touch;

	struct wl_event_source *display_source;
};

extern struct waylandnode *wayland_create_node(struct nemocompz *compz, const char *name);
extern void wayland_destroy_node(struct waylandnode *node);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
