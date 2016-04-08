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
#include <actor.h>
#include <view.h>
#include <layer.h>
#include <region.h>
#include <scaler.h>
#include <presentation.h>
#include <subcompz.h>
#include <seat.h>
#include <touch.h>
#include <session.h>
#include <task.h>
#include <sound.h>
#include <clipboard.h>
#include <virtuio.h>
#include <screen.h>
#include <animation.h>
#include <effect.h>
#include <fbbackend.h>
#include <waylandbackend.h>
#include <evdevbackend.h>
#include <tuiobackend.h>
#include <nemomisc.h>
#include <nemoease.h>
#include <nemoxml.h>
#include <nemolog.h>

#ifdef NEMOUX_WITH_DRM
#include <drmbackend.h>
#endif

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

	resource = wl_resource_create(client, &wl_compositor_interface, MIN(version, 3), id);
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

	if (compz->display != NULL) {
		nemocompz_exit(compz);
	}

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
		wl_signal_emit(&compz->child_signal, &proc);

		wl_list_for_each(task, &compz->task_list, link) {
			if (task->pid == pid)
				break;
		}

		if (&task->link != &compz->task_list) {
			wl_list_remove(&task->link);

			task->cleanup(task, state);
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

	debug_show_backtrace();

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
	char *dir = getenv("XDG_RUNTIME_DIR");
	struct stat st;

	if (dir == NULL) {
		setenv("XDG_RUNTIME_DIR", "/tmp", 1);

		dir = getenv("XDG_RUNTIME_DIR");
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
	char *ename = getenv("WAYLAND_DISPLAY");
	char *dname;
	int i;

	for (i = 0; i < 8; i++) {
		asprintf(&dname, "wayland-%d", i);
		if (ename == NULL || strcmp(ename, dname) != 0) {
			break;
		}

		free(dname);
	}

	return dname;
}

static inline void nemocompz_dispatch_actor_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoactor *actor, *next;

	wl_list_for_each_safe(actor, next, &compz->frame_list, frame_link) {
		actor->dispatch_frame(actor, msecs);
	}
}

static inline void nemocompz_dispatch_animation_frame(struct nemocompz *compz, uint32_t msecs)
{
	struct nemoanimation *anim, *next;
	double progress;

	wl_list_for_each_safe(anim, next, &compz->animation_list, link) {
		if (msecs < anim->stime)
			continue;

		anim->frame_count++;

		if (msecs >= anim->etime) {
			progress = 1.0f;
		} else {
			progress = nemoease_get(&anim->ease, msecs - anim->stime, anim->duration);
		}

		anim->frame(anim, progress);

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

	nemocompz_dispatch_actor_frame(compz, msecs);
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

static int nemocompz_dispatch_touch_timeout(void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct nemoseat *seat = compz->seat;
	uint32_t msecs = time_current_msecs();

	nemoseat_update_touchpoints(seat, msecs);

	wl_event_source_timer_update(compz->touch_timer, compz->touch_timeout);
}

static int nemocompz_dispatch_framerate_timeout(void *data)
{
	struct nemocompz *compz = (struct nemocompz *)data;
	struct nemoseat *seat = compz->seat;
	struct nemoscreen *screen;
	struct nemotouch *touch;
	uint32_t msecs = time_current_msecs();

	nemolog_message("COMPZ", "%fs frametime\n", (double)msecs / 1000.0f);

	wl_list_for_each(screen, &compz->screen_list, link) {
		nemolog_message("COMPZ", "  [%d:%d] screen %u frames...\n", screen->node->nodeid, screen->screenid, screen->frame_count);

		screen->frame_count = 0;
	}

	wl_list_for_each(touch, &seat->touch.device_list, link) {
		nemolog_message("COMPZ", "  [%s] touch %u frames...(%d)\n", touch->node->devnode, touch->frame_count, !wl_list_empty(&touch->touchpoint_list));

		touch->frame_count = 0;
	}

	wl_event_source_timer_update(compz->framerate_timer, 1000);
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
	char *env;

	compz = (struct nemocompz *)malloc(sizeof(struct nemocompz));
	if (compz == NULL)
		return NULL;
	memset(compz, 0, sizeof(struct nemocompz));

	compz->configs = nemoitem_create(64);
	if (compz->configs == NULL)
		goto err1;

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
	compz->stick_ids = 0;
	compz->touch_ids = NEMOCOMPZ_POINTER_MAX;
	compz->nodemax = NEMOCOMPZ_NODE_MAX;
	compz->name = nemocompz_get_display_name();

	wl_signal_init(&compz->destroy_signal);

	wl_list_init(&compz->key_binding_list);
	wl_list_init(&compz->button_binding_list);
	wl_list_init(&compz->touch_binding_list);

	wl_list_init(&compz->backend_list);
	wl_list_init(&compz->screen_list);
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
	wl_list_init(&compz->actor_list);
	wl_list_init(&compz->feedback_list);

	wl_signal_init(&compz->session_signal);
	compz->session_active = 1;

	wl_signal_init(&compz->show_input_panel_signal);
	wl_signal_init(&compz->hide_input_panel_signal);
	wl_signal_init(&compz->update_input_panel_signal);

	wl_signal_init(&compz->create_surface_signal);
	wl_signal_init(&compz->activate_signal);
	wl_signal_init(&compz->transform_signal);
	wl_signal_init(&compz->kill_signal);
	wl_signal_init(&compz->child_signal);

	nemolayer_prepare(&compz->cursor_layer, &compz->layer_list);

	pixman_region32_init(&compz->damage);
	pixman_region32_init(&compz->scene);
	pixman_region32_init(&compz->scope);

	compz->udev = udev_new();
	if (compz->udev == NULL)
		goto err1;

	compz->session = nemosession_create(compz);
	if (compz->session == NULL)
		goto err1;

	if (!wl_global_create(compz->display, &wl_compositor_interface, 3, compz, nemocompz_bind_compositor))
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

	compz->touch_timer = wl_event_loop_add_timer(compz->loop, nemocompz_dispatch_touch_timeout, compz);
	if (compz->touch_timer == NULL)
		goto err1;

	compz->touch_timeout = NEMOCOMPZ_DEFAULT_TOUCH_TIMEOUT;

	wl_event_source_timer_update(compz->touch_timer, compz->touch_timeout);

	env = getenv("NEMOUX_FRAMERATE_LOG");
	if (env != NULL && strcmp(env, "ON") == 0) {
		compz->framerate_timer = wl_event_loop_add_timer(compz->loop, nemocompz_dispatch_framerate_timeout, compz);
		if (compz->framerate_timer == NULL)
			goto err1;

		wl_event_source_timer_update(compz->framerate_timer, 1000);
	}

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

	if (compz->touch_timer != NULL)
		wl_event_source_remove(compz->touch_timer);

	if (compz->framerate_timer != NULL)
		wl_event_source_remove(compz->framerate_timer);

	nemolog_message("COMPZ", "destroy current session\n");

	if (compz->session != NULL)
		nemosession_destroy(compz->session);

	nemolog_message("COMPZ", "destroy wayland display\n");

	if (compz->display != NULL)
		wl_display_destroy(compz->display);

	if (compz->configs != NULL)
		nemoitem_destroy(compz->configs);

	free(compz);
}

int nemocompz_run(struct nemocompz *compz)
{
	if (compz->state == NEMOCOMPZ_RUNNING_STATE) {
		setenv("WAYLAND_DISPLAY", compz->name, 1);

		wl_display_run(compz->display);
	}

	return 0;
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

void nemocompz_acculumate_damage(struct nemocompz *compz)
{
	struct nemoscreen *screen;
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

	wl_list_for_each(screen, &compz->screen_list, link) {
		pixman_region32_union(&screen->damage, &screen->damage, &compz->damage);
		pixman_region32_intersect(&screen->damage, &screen->damage, &screen->region);
	}

	pixman_region32_clear(&compz->damage);
}

void nemocompz_flush_damage(struct nemocompz *compz)
{
	struct nemocanvas *canvas;
	struct nemoactor *actor;

	wl_list_for_each(canvas, &compz->canvas_list, link) {
		if (canvas->base.dirty != 0)
			nemocanvas_flush_damage(canvas);
	}

	wl_list_for_each(actor, &compz->actor_list, link) {
		if (actor->base.dirty != 0)
			nemoactor_flush_damage(actor);
	}
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

	if (compz->has_scene != 0)
		return;

	pixman_region32_clear(&compz->scene);

	wl_list_for_each(screen, &compz->screen_list, link) {
		pixman_region32_union(&compz->scene, &compz->scene, &screen->region);
	}

	if (compz->has_scope == 0)
		pixman_region32_copy(&compz->scope, &compz->scene);
}

void nemocompz_set_scene(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&compz->scene, x, y, width, height);

	compz->has_scene = 1;

	if (compz->has_scope == 0)
		pixman_region32_init_rect(&compz->scope, x, y, width, height);
}

void nemocompz_set_scope(struct nemocompz *compz, int32_t x, int32_t y, int32_t width, int32_t height)
{
	pixman_region32_init_rect(&compz->scope, x, y, width, height);

	compz->has_scope = 1;
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

					if (child->transform.done != 0) {
						nemoview_update_transform_done(child);
					}
				}
			}

			if (view->transform.dirty != 0) {
				nemoview_update_transform(view);
			}

			if (view->transform.done != 0) {
				nemoview_update_transform_done(view);
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

	if (pixman_region32_contains_rectangle(region, pixman_region32_extents(&cview->geometry.scope)) == PIXMAN_REGION_OUT)
		goto out;

	wl_list_for_each(layer, &compz->layer_list, link) {
		wl_list_for_each(view, &layer->view_list, layer_link) {
			if (cview == view)
				goto out;

			if (!wl_list_empty(&view->children_list)) {
				wl_list_for_each(child, &view->children_list, children_link) {
					if (cview == child)
						goto out;

					if (nemoview_has_state_all(child, NEMO_VIEW_LAYER_STATE | NEMO_VIEW_OPAQUE_STATE) &&
							nemoview_contain_view(child, cview) != 0)
						return -1;

					if (nemoview_has_state_all(child, NEMO_VIEW_LAYER_STATE) &&
							nemoview_overlap_view(cview, child) != 0)
						visible = 0;
				}
			}

			if (nemoview_has_state(view, NEMO_VIEW_LAYER_STATE | NEMO_VIEW_OPAQUE_STATE) &&
					nemoview_contain_view(view, cview) != 0)
				return -1;

			if (nemoview_has_state(view, NEMO_VIEW_LAYER_STATE) &&
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
					if (nemoview_has_state(child, NEMO_VIEW_LAYER_STATE)) {
						nemocontent_update_layer(child->content, nemocompz_check_layer(compz, child, &region));

						pixman_region32_union(&region, &region, &child->geometry.scope);
					}
				}
			}

			if (nemoview_has_state(view, NEMO_VIEW_LAYER_STATE)) {
				nemocontent_update_layer(view->content, nemocompz_check_layer(compz, view, &region));

				pixman_region32_union(&region, &region, &view->geometry.scope);
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

void nemocompz_set_touch_timeout(struct nemocompz *compz, uint32_t timeout)
{
	compz->touch_timeout = timeout;
}

struct nemoeventqueue *nemocompz_get_main_eventqueue(struct nemocompz *compz)
{
	if (compz->queue == NULL)
		compz->queue = nemoeventqueue_create(compz);

	return compz->queue;
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

void nemocompz_load_configs(struct nemocompz *compz, const char *configpath)
{
	struct nemoxml *xml;
	struct xmlnode *node;
	int i, j;

	xml = nemoxml_create();
	nemoxml_load_file(xml, configpath);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		i = nemoitem_set(compz->configs, node->path);

		for (j = 0; j < node->nattrs; j++) {
			nemoitem_set_attr(compz->configs, i,
					node->attrs[j*2+0],
					node->attrs[j*2+1]);
		}
	}

	nemoxml_destroy(xml);
}

void nemocompz_load_backends(struct nemocompz *compz)
{
	const char *name;
	int index;

	nemoitem_for_each(compz->configs, index, "//nemoshell/backend", 0) {
		name = nemoitem_get_attr(compz->configs, index, "name");
		if (name == NULL)
			continue;

		if (strcmp(name, "drm") == 0) {
			drmbackend_create(compz, index);
		} else if (strcmp(name, "fb") == 0) {
			fbbackend_create(compz, index);
		} else if (strcmp(name, "evdev") == 0) {
			evdevbackend_create(compz, index);
		} else if (strcmp(name, "tuio") == 0) {
			tuiobackend_create(compz, index);
		}
	}
}

void nemocompz_load_scenes(struct nemocompz *compz)
{
	int index;

	index = nemoitem_get(compz->configs, "//nemoshell/scene", 0);
	if (index >= 0) {
		int32_t x, y;
		int32_t width, height;

		x = nemoitem_get_iattr(compz->configs, index, "x", 0);
		y = nemoitem_get_iattr(compz->configs, index, "y", 0);
		width = nemoitem_get_iattr(compz->configs, index, "width", 0);
		height = nemoitem_get_iattr(compz->configs, index, "height", 0);

		nemocompz_set_scene(compz, x, y, width, height);
	}

	index = nemoitem_get(compz->configs, "//nemoshell/scope", 0);
	if (index >= 0) {
		int32_t x, y;
		int32_t width, height;

		x = nemoitem_get_iattr(compz->configs, index, "x", 0);
		y = nemoitem_get_iattr(compz->configs, index, "y", 0);
		width = nemoitem_get_iattr(compz->configs, index, "width", 0);
		height = nemoitem_get_iattr(compz->configs, index, "height", 0);

		nemocompz_set_scope(compz, x, y, width, height);
	}
}

void nemocompz_load_virtuios(struct nemocompz *compz)
{
	int index, i;
	int port, fps;
	int x, y, width, height;

	nemoitem_for_each(compz->configs, index, "//nemoshell/virtuio", 0) {
		port = nemoitem_get_iattr(compz->configs, index, "port", 3333);
		fps = nemoitem_get_iattr(compz->configs, index, "fps", 60);
		x = nemoitem_get_iattr(compz->configs, index, "x", 0);
		y = nemoitem_get_iattr(compz->configs, index, "y", 0);
		width = nemoitem_get_iattr(compz->configs, index, "width", nemocompz_get_scene_width(compz));
		height = nemoitem_get_iattr(compz->configs, index, "height", nemocompz_get_scene_height(compz));

		virtuio_create(compz, port, fps, x, y, width, height);
	}
}
