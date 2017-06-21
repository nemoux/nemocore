#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <wayland-server.h>
#include <wayland-scaler-server-protocol.h>
#include <wayland-presentation-timing-server-protocol.h>

#include <compz.h>
#include <datadevice.h>
#include <canvas.h>
#include <view.h>
#include <layer.h>
#include <region.h>
#include <scaler.h>
#include <presentation.h>
#include <subcompz.h>
#include <seat.h>
#include <touch.h>
#include <session.h>
#include <sound.h>
#include <clipboard.h>
#include <virtuio.h>
#include <screen.h>
#include <input.h>
#include <animation.h>
#include <effect.h>
#include <drmbackend.h>
#include <fbbackend.h>
#include <evdevbackend.h>
#include <evdevnode.h>
#include <tuio.h>
#include <nemomisc.h>
#include <nemoease.h>
#include <nemoxml.h>
#include <nemolog.h>

static void compositor_create_surface(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id)
{
	nemocanvas_create(client, compositor_resource, id);
}

static void compositor_create_region(struct wl_client *client, struct wl_resource *compositor_resource, uint32_t id)
{
	nemoregion_create(client, compositor_resource, id);
}

static const struct wl_compositor_interface compositor_implementation = {
	compositor_create_surface,
	compositor_create_region
};

static void nemocompz_bind_compositor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct wl_resource *resource;

	resource = wl_resource_create(client, &wl_compositor_interface, version, id);
	if (resource == NULL) {
		wl_client_post_no_memory(client);
		return;
	}

	wl_resource_set_implementation(resource, &compositor_implementation, compz, NULL);
}

static void nemocompz_bind_subcompositor(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemosubcompz_bind(client, data, version, id);
}

static void nemocompz_bind_scaler(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemoscaler_bind(client, data, version, id);
}

static void nemocompz_bind_presentation(struct wl_client *client, void *data, uint32_t version, uint32_t id)
{
	nemopresentation_bind(client, data, version, id);
}

static int on_term_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;

	nemolog_error("COMPZ", "received %d term signal\n", signum);

	if (compz->display != NULL)
		nemocompz_exit(compz);

	return 1;
}

static int on_chld_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct nemotask *task;
	struct nemoproc proc;
	pid_t pid;
	int state;

	nemolog_message("COMPZ", "received %d child signal\n", signum);

	while (1) {
		pid = waitpid(-1, &state, WNOHANG);
		if (pid < 0)
			return 1;
		if (pid == 0)
			return 1;

		proc.pid = pid;
		wl_signal_emit(&compz->sigchld_signal, &proc);

		wl_list_for_each(task, &compz->task_list, link) {
			if (task->pid == pid)
				break;
		}

		if (&task->link != &compz->task_list) {
			wl_list_remove(&task->link);

			task->cleanup(compz, task, state);
		}
	}

	return 1;
}

static int on_fault_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;

	nemolog_error("COMPZ", "received %d fault signal\n", signum);

	if (compz->display != NULL) {
		nemocompz_exit(compz);
	}

	return 1;
}

static int on_pipe_signal(int signum, void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;

	nemolog_error("COMPZ", "received %d fault signal\n", signum);

	return 1;
}

static int nemocompz_check_xdg_runtime_dir(void)
{
	const char *dir = env_get_string("XDG_RUNTIME_DIR", NULL);
	struct stat st;

	if (dir == NULL) {
		env_set_string("XDG_RUNTIME_DIR", "/tmp");

		dir = env_get_string("XDG_RUNTIME_DIR", NULL);
		if (dir == NULL)
			return -1;
	}

	if (stat(dir, &st) || !S_ISDIR(st.st_mode)) {
		return -1;
	}

	if ((st.st_mode & 0777) != 0700 || st.st_uid != getuid()) {
		return -1;
	}

	return 0;
}

static char *nemocompz_get_display_name(void)
{
	const char *ename = env_get_string("WAYLAND_DISPLAY", NULL);
	char *dname;
	int i;

	for (i = 0; i < 8; i++) {
		asprintf(&dname, "wayland-%d", i);
		if (ename == NULL || strcmp(ename, dname) == 0) {
			break;
		}

		free(dname);
	}

	return dname;
}

static inline void nemocompz_dispatch_animation_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoanimation *anim, *next;

	wl_list_for_each_safe(anim, next, &compz->animation_list, link) {
		if (msecs < anim->stime)
			continue;

		anim->frame_count++;

		anim->frame(anim,
				nemoease_get(&anim->ease, msecs - anim->stime, anim->duration));

		if (msecs >= anim->etime) {
			wl_list_remove(&anim->link);
			wl_list_init(&anim->link);

			if (anim->done != NULL)
				anim->done(anim);
		}
	}
}

static inline void nemocompz_dispatch_effect_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoeffect *effect, *next;
	int done;

	wl_list_for_each_safe(effect, next, &compz->effect_list, link) {
		effect->frame_count++;

		done = effect->frame(effect, msecs - effect->ptime);
		if (done != 0) {
			wl_list_remove(&effect->link);
			wl_list_init(&effect->link);

			if (effect->done != NULL)
				effect->done(effect);
		} else {
			effect->ptime = msecs;
		}
	}
}

static int nemocompz_dispatch_frame_timeout(void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	uint32_t msecs = time_current_msecs();

	if (compz->scene_dirty != 0)
		nemocompz_update_scene(compz);

	nemocompz_dispatch_animation_frame(compz, msecs);
	nemocompz_dispatch_effect_frame(compz, msecs);

	if (!wl_list_empty(&compz->frame_list) ||
			!wl_list_empty(&compz->animation_list) ||
			!wl_list_empty(&compz->effect_list)) {
		wl_event_source_timer_update(compz->frame_timer, compz->frame_timeout);
	} else {
		compz->frame_done = 1;
	}

	return 1;
}

void nemocompz_dispatch_frame(struct nemocompz *compz)
{
	if (compz->frame_done != 0) {
		wl_event_source_timer_update(compz->frame_timer, compz->frame_timeout);

		compz->frame_done = 0;
	}
}

void nemocompz_dispatch_idle(struct nemocompz *compz, nemocompz_dispatch_idle_t dispatch, void *data)
{
	wl_event_loop_add_idle(compz->loop, dispatch, data);
}

struct nemocompz *nemocompz_create(void)
{
	struct nemocompz *compz;
	struct wl_display *display;
	struct wl_event_loop *loop;
	int compositor_version = env_get_integer("WAYLAND_COMPOSITOR_VERSION", NEMOUX_WAYLAND_COMPOSITOR_VERSION);

	compz = (struct nemocompz *)malloc(sizeof(struct nemocompz));
	if (compz == NULL)
		return NULL;
	memset(compz, 0, sizeof(struct nemocompz));

	display = wl_display_create();
	if (display == NULL)
		goto err1;

	nemocompz_check_xdg_runtime_dir();

	loop = wl_display_get_event_loop(display);

	compz->sigsrc[0] = wl_event_loop_add_signal(loop, SIGTERM, on_term_signal, compz);
	compz->sigsrc[1] = wl_event_loop_add_signal(loop, SIGINT, on_term_signal, compz);
	compz->sigsrc[2] = wl_event_loop_add_signal(loop, SIGQUIT, on_term_signal, compz);
	compz->sigsrc[3] = wl_event_loop_add_signal(loop, SIGCHLD, on_chld_signal, compz);
	compz->sigsrc[4] = wl_event_loop_add_signal(loop, SIGSEGV, on_fault_signal, compz);
	compz->sigsrc[5] = wl_event_loop_add_signal(loop, SIGPIPE, on_pipe_signal, compz);

	compz->display = display;
	compz->loop = loop;
	compz->state = NEMOCOMPZ_RUNNING_STATE;
	compz->use_pixman = 0;
	compz->screen_idpool = 0;
	compz->keyboard_ids = 0;
	compz->pointer_ids = 0;
	compz->touch_ids = NEMOCOMPZ_POINTER_MAX;
	compz->nodemax = NEMOCOMPZ_NODE_MAX;
	compz->name = nemocompz_get_display_name();

	wl_signal_init(&compz->destroy_signal);

	wl_list_init(&compz->key_binding_list);
	wl_list_init(&compz->button_binding_list);
	wl_list_init(&compz->touch_binding_list);

	wl_list_init(&compz->backend_list);
	wl_list_init(&compz->screen_list);
	wl_list_init(&compz->input_list);
	wl_list_init(&compz->render_list);
	wl_list_init(&compz->evdev_list);
	wl_list_init(&compz->touch_list);
	wl_list_init(&compz->tuio_list);
	wl_list_init(&compz->virtuio_list);
	wl_list_init(&compz->animation_list);
	wl_list_init(&compz->effect_list);
	wl_list_init(&compz->task_list);
	wl_list_init(&compz->layer_list);
	wl_list_init(&compz->canvas_list);
	wl_list_init(&compz->feedback_list);

	wl_signal_init(&compz->session_signal);
	compz->session_active = 1;

	wl_signal_init(&compz->create_surface_signal);
	wl_signal_init(&compz->activate_signal);
	wl_signal_init(&compz->transform_signal);
	wl_signal_init(&compz->sigchld_signal);

	compz->cursor_layer = nemolayer_create(compz, "cursor");
	nemolayer_attach_above(compz->cursor_layer, NULL);

	pixman_region32_init(&compz->damage);
	pixman_region32_init(&compz->scene);
	pixman_region32_init(&compz->scope);

	compz->udev = udev_new();
	if (compz->udev == NULL)
		goto err1;

	compz->session = nemosession_create(compz);
	if (compz->session == NULL)
		goto err1;

	if (!wl_global_create(compz->display, &wl_compositor_interface, compositor_version, compz, nemocompz_bind_compositor))
		goto err1;

	if (!wl_global_create(compz->display, &wl_subcompositor_interface, 1, compz, nemocompz_bind_subcompositor))
		goto err1;

	if (!wl_global_create(compz->display, &wl_scaler_interface, 2, compz, nemocompz_bind_scaler))
		goto err1;

	if (!wl_global_create(compz->display, &presentation_interface, 1, compz, nemocompz_bind_presentation))
		goto err1;

	datadevice_manager_init(compz->display);
	wl_display_init_shm(compz->display);

	compz->seat = nemoseat_create(compz);
	if (compz->seat == NULL)
		goto err1;

	compz->sound = nemosound_create(compz);
	if (compz->sound == NULL)
		goto err1;

	clipboard_create(compz->seat);

	if (wl_display_add_socket(compz->display, compz->name))
		goto err1;

	compz->frame_timer = wl_event_loop_add_timer(compz->loop, nemocompz_dispatch_frame_timeout, compz);
	if (compz->frame_timer == NULL)
		goto err1;
	compz->frame_timeout = NEMOCOMPZ_DEFAULT_FRAME_TIMEOUT;
	compz->frame_done = 1;

	wl_list_init(&compz->frame_list);

	return compz;

err1:
	nemocompz_destroy(compz);

	return NULL;
}

void nemocompz_destroy(struct nemocompz *compz)
{
	struct nemobackend *backend, *bnext;
	struct nemoscreen *screen, *snext;
	int i;

	nemolog_message("COMPZ", "emit compz's destroy signal\n");

	wl_signal_emit(&compz->destroy_signal, compz);

	nemolog_message("COMPZ", "destroy all screens\n");

	wl_list_for_each_safe(screen, snext, &compz->screen_list, link) {
		if (screen->destroy != NULL)
			screen->destroy(screen);
	}

	nemolog_message("COMPZ", "destroy all backends\n");

	wl_list_for_each_safe(backend, bnext, &compz->backend_list, link) {
		if (backend->destroy != NULL)
			backend->destroy(backend);
	}

	pixman_region32_fini(&compz->damage);
	pixman_region32_fini(&compz->scene);
	pixman_region32_fini(&compz->scope);

	if (compz->udev != NULL)
		udev_unref(compz->udev);

	if (compz->seat != NULL)
		nemoseat_destroy(compz->seat);

	if (compz->sound != NULL)
		nemosound_destroy(compz->sound);

	if (compz->name != NULL)
		free(compz->name);

	nemolog_message("COMPZ", "destroy event sources\n");

	for (i = 0; i < ARRAY_LENGTH(compz->sigsrc); i++) {
		if (compz->sigsrc[i] != NULL)
			wl_event_source_remove(compz->sigsrc[i]);
	}

	if (compz->frame_timer != NULL)
		wl_event_source_remove(compz->frame_timer);

	nemolog_message("COMPZ", "destroy current session\n");

	if (compz->session != NULL)
		nemosession_destroy(compz->session);

	nemolog_message("COMPZ", "destroy wayland display\n");

	if (compz->cursor_layer != NULL)
		nemolayer_destroy(compz->cursor_layer);

	if (compz->display != NULL)
		wl_display_destroy(compz->display);

	free(compz);
}

int nemocompz_run(struct nemocompz *compz)
{
	if (compz->state == NEMOCOMPZ_RUNNING_STATE) {
		env_set_string("WAYLAND_DISPLAY", compz->name);

		wl_display_run(compz->display);
	}

	return compz->retval;
}

void nemocompz_exit(struct nemocompz *compz)
{
	compz->state = NEMOCOMPZ_EXIT_STATE;

	compz->session_active = 0;
	wl_signal_emit(&compz->session_signal, compz);

	wl_display_terminate(compz->display);
}

void nemocompz_destroy_clients(struct nemocompz *compz)
{
	struct nemocanvas *canvas;
	struct wl_client *client;
	pid_t cid = getpid();
	pid_t pid;

	while (!wl_list_empty(&compz->canvas_list)) {
		canvas = (struct nemocanvas *)container_of(compz->canvas_list.next, struct nemocanvas, link);

		wl_list_remove(&canvas->link);
		wl_list_init(&canvas->link);

		if ((canvas != NULL) && (canvas->resource != NULL) &&
				(client = wl_resource_get_client(canvas->resource)) != NULL) {
			wl_client_get_credentials(client, &pid, NULL, NULL);

			if (cid != pid)
				kill(pid, SIGKILL);
			else
				wl_client_destroy(client);
		}
	}
}

void nemocompz_accumulate_damage(struct nemocompz *compz)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;
	pixman_region32_t opaque;

	pixman_region32_init(&opaque);

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					nemoview_accumulate_damage(child, &opaque);
				}
			}

			nemoview_accumulate_damage(view, &opaque);
		}
	}

	pixman_region32_fini(&opaque);
}

void nemocompz_flush_damage(struct nemocompz *compz)
{
	struct nemocanvas *canvas;
	struct nemoscreen *screen;

	wl_list_for_each(canvas, &compz->canvas_list, link) {
		if (canvas->base.dirty != 0)
			nemocanvas_flush_damage(canvas);
	}

	wl_list_for_each(screen, &compz->screen_list, link) {
		if (nemoscreen_has_state(screen, NEMOSCREEN_OVERLAY_STATE) == 0) {
			pixman_region32_union(&screen->damage, &screen->damage, &compz->damage);
			pixman_region32_intersect(&screen->damage, &screen->damage, &screen->region);
		} else {
			nemoview_merge_damage(screen->overlay, &screen->damage);
		}
	}

	wl_list_for_each(canvas, &compz->canvas_list, link) {
		if (canvas->base.dirty != 0) {
			canvas->base.dirty = 0;

			pixman_region32_clear(&canvas->base.damage);
		}
	}

	pixman_region32_clear(&compz->damage);
}

void nemocompz_make_current(struct nemocompz *compz)
{
	if (compz->renderer != NULL && compz->renderer->make_current != NULL)
		compz->renderer->make_current(compz->renderer);
}

struct nemoscreen *nemocompz_get_main_screen(struct nemocompz *compz)
{
	if (wl_list_empty(&compz->screen_list))
		return NULL;

	return (struct nemoscreen *)container_of(compz->screen_list.next, struct nemoscreen, link);
}

struct nemoscreen *nemocompz_get_screen_on(struct nemocompz *compz, float x, float y)
{
	struct nemoscreen *screen;

	wl_list_for_each(screen, &compz->screen_list, link) {
		if (pixman_region32_contains_point(&screen->region, x, y, NULL)) {
			return screen;
		}
	}

	return NULL;
}

struct nemoscreen *nemocompz_get_screen(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid)
{
	struct nemoscreen *screen;

	wl_list_for_each(screen, &compz->screen_list, link) {
		if (screen->screenid == screenid && screen->node->nodeid == nodeid) {
			return screen;
		}
	}

	return NULL;
}

struct inputnode *nemocompz_get_input(struct nemocompz *compz, const char *devnode, uint32_t type)
{
	struct inputnode *node;

	wl_list_for_each(node, &compz->input_list, link) {
		if (strcmp(node->devnode, devnode) == 0 && nemoinput_has_type(node, type) != 0)
			return node;
	}

	return NULL;
}

struct evdevnode *nemocompz_get_evdev(struct nemocompz *compz, const char *devpath)
{
	struct evdevnode *node;

	wl_list_for_each(node, &compz->evdev_list, link) {
		if (strcmp(node->devpath, devpath) == 0)
			return node;
	}

	return NULL;
}

int32_t nemocompz_get_scene_width(struct nemocompz *compz)
{
	pixman_box32_t *extents;

	extents = pixman_region32_extents(&compz->scene);

	return extents->x2;
}

int32_t nemocompz_get_scene_height(struct nemocompz *compz)
{
	pixman_box32_t *extents;

	extents = pixman_region32_extents(&compz->scene);

	return extents->y2;
}

void nemocompz_update_scene(struct nemocompz *compz)
{
	struct nemoscreen *screen;

	compz->scene_dirty = 0;

	if (compz->has_scene == 0) {
		pixman_region32_clear(&compz->scene);

		wl_list_for_each(screen, &compz->screen_list, link) {
			pixman_region32_union(&compz->scene, &compz->scene, &screen->region);
		}
	}

	if (compz->has_scope == 0) {
		pixman_region32_clear(&compz->scope);

		wl_list_for_each(screen, &compz->screen_list, link) {
			if (nemoscreen_has_state(screen, NEMOSCREEN_SCOPE_STATE) != 0)
				pixman_region32_union(&compz->scope, &compz->scope, &screen->region);
		}
	}

	if (compz->has_output == 0) {
		pixman_box32_t *extents = pixman_region32_extents(&compz->scene);

		compz->output.x = 0;
		compz->output.y = 0;
		compz->output.width = extents->x2 - extents->x1;
		compz->output.height = extents->y2 - extents->y1;
	}
}

void nemocompz_scene_dirty(struct nemocompz *compz)
{
	compz->scene_dirty = 1;

	nemocompz_dispatch_frame(compz);
}

void nemocompz_set_scene(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&compz->scene, x, y, width, height);

	compz->has_scene = 1;
}

void nemocompz_set_scope(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&compz->scope, x, y, width, height);

	compz->has_scope = 1;
}

void nemocompz_set_output(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height)
{
	compz->output.x = x;
	compz->output.y = y;
	compz->output.width = width;
	compz->output.height = height;

	compz->has_output = 1;
}

void nemocompz_update_transform(struct nemocompz *compz)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (child->transform.dirty != 0) {
						nemoview_update_transform(child);
					}

					if (child->transform.notify != 0) {
						nemoview_update_transform_notify(child);

						wl_signal_emit(&compz->transform_signal, child);
					}
				}
			}

			if (view->transform.dirty != 0) {
				nemoview_update_transform(view);
			}

			if (view->transform.notify != 0) {
				nemoview_update_transform_notify(view);

				wl_signal_emit(&compz->transform_signal, view);
			}
		}
	}
}

void nemocompz_update_output(struct nemocompz *compz)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					nemoview_update_output(child);
				}
			}

			nemoview_update_output(view);
		}
	}
}

static inline int nemocompz_check_layer(struct nemocompz *compz, struct nemoview *cview, pixman_region32_t *region)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;
	int visible = 1;

	if (pixman_region32_contains_rectangle(region, pixman_region32_extents(&cview->geometry.region)) == PIXMAN_REGION_OUT)
		goto out;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (cview == view)
				goto out;

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (cview == child)
						goto out;

					if (nemoview_has_state_all(child, NEMOVIEW_LAYER_STATE | NEMOVIEW_OPAQUE_STATE) &&
							nemoview_contain_view(child, cview) != 0)
						return -1;

					if (nemoview_has_state_all(child, NEMOVIEW_LAYER_STATE) &&
							nemoview_overlap_view(cview, child) != 0)
						visible = 0;
				}
			}

			if (nemoview_has_state_all(view, NEMOVIEW_LAYER_STATE | NEMOVIEW_OPAQUE_STATE) &&
					nemoview_contain_view(view, cview) != 0)
				return -1;

			if (nemoview_has_state_all(view, NEMOVIEW_LAYER_STATE) &&
					nemoview_overlap_view(cview, view) != 0)
				visible = 0;
		}
	}

out:
	return visible;
}

void nemocompz_update_layer(struct nemocompz *compz)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;
	pixman_region32_t region;

	pixman_region32_init(&region);

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (nemoview_has_state(child, NEMOVIEW_LAYER_STATE)) {
						nemocontent_update_layer(child->content, nemocompz_check_layer(compz, child, &region));

						pixman_region32_union(&region, &region, &child->geometry.region);
					}
				}
			}

			if (nemoview_has_state(view, NEMOVIEW_LAYER_STATE)) {
				nemocontent_update_layer(view->content, nemocompz_check_layer(compz, view, &region));

				pixman_region32_union(&region, &region, &view->geometry.region);
			}
		}
	}

	pixman_region32_fini(&region);
}

void nemocompz_dispatch_animation(struct nemocompz *compz, struct nemoanimation *animation)
{
	if (animation->frame == NULL || animation->duration == 0)
		return;

	wl_list_insert(&compz->animation_list, &animation->link);

	animation->frame_count = 0;
	animation->stime = time_current_msecs() + animation->delay;
	animation->etime = animation->stime + animation->duration;

	nemocompz_dispatch_frame(compz);
}

void nemocompz_dispatch_effect(struct nemocompz *compz, struct nemoeffect *effect)
{
	if (effect->frame == NULL)
		return;

	wl_list_insert(&compz->effect_list, &effect->link);

	effect->frame_count = 0;
	effect->stime = time_current_msecs();
	effect->ptime = effect->stime;

	nemocompz_dispatch_frame(compz);
}

void nemocompz_set_frame_timeout(struct nemocompz *compz, uint32_t timeout)
{
	compz->frame_timeout = timeout;
}

int nemocompz_set_presentation_clock(struct nemocompz *compz, clockid_t id)
{
	struct timespec ts;

	if (clock_gettime(id, &ts) < 0)
		return -1;

	compz->presentation_clock = id;

	return 0;
}

int nemocompz_set_presentation_clock_software(struct nemocompz *compz)
{
	static const clockid_t clocks[] = {
		CLOCK_MONOTONIC_RAW,
		CLOCK_MONOTONIC_COARSE,
		CLOCK_MONOTONIC,
		CLOCK_REALTIME_COARSE,
		CLOCK_REALTIME
	};
	int i;

	for (i = 0; i < ARRAY_LENGTH(clocks); i++) {
		if (nemocompz_set_presentation_clock(compz, clocks[i]) == 0)
			return 0;
	}

	return -1;
}

void nemocompz_get_presentation_clock(struct nemocompz *compz, struct timespec *ts)
{
	if (clock_gettime(compz->presentation_clock, ts) < 0) {
		ts->tv_sec = 0;
		ts->tv_nsec = 0;
	}
}

int nemocompz_is_running(struct nemocompz *compz)
{
	return compz->state == NEMOCOMPZ_RUNNING_STATE;
}

int nemocompz_contain_view(struct nemocompz *compz, struct nemoview *view)
{
	float tx, ty;

	nemoview_transform_to_global(view,
			view->content->width * view->geometry.fx,
			view->content->height * view->geometry.fy,
			&tx, &ty);

	if (!pixman_region32_contains_point(&compz->scope, tx, ty, NULL))
		return 0;

	return 1;
}

int nemocompz_contain_view_near(struct nemocompz *compz, struct nemoview *view, float dx, float dy)
{
	float tx, ty;

	nemoview_transform_to_global(view,
			view->content->width * view->geometry.fx,
			view->content->height * view->geometry.fy,
			&tx, &ty);

	if (!pixman_region32_contains_point(&compz->scope, tx + dx, ty + dy, NULL))
		return 0;

	return 1;
}

struct nemoview *nemocompz_get_view_by_uuid(struct nemocompz *compz, const char *uuid)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (strcmp(view->uuid, uuid) == 0)
				return view;

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (strcmp(child->uuid, uuid) == 0)
						return child;
				}
			}
		}
	}

	return NULL;
}

struct nemoview *nemocompz_get_view_by_client(struct nemocompz *compz, struct wl_client *client)
{
	struct nemolayer *layer;
	struct nemoview *view, *child;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (view->client == client)
				return view;

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (child->client == client)
						return child;
				}
			}
		}
	}

	return NULL;
}

struct nemolayer *nemocompz_get_layer_by_name(struct nemocompz *compz, const char *name)
{
	struct nemolayer *layer;

	wl_list_for_each(layer, &compz->layer_list, link) {
		if (strcmp(layer->name, name) == 0)
			return layer;
	}

	return NULL;
}

void nemocompz_watch_task(struct nemocompz *compz, struct nemotask *task)
{
	wl_list_insert(&compz->task_list, &task->link);
}
