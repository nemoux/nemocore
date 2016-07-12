#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <nemoshow.h>
#include <showcanvas.h>
#include <showcanvas.hpp>
#include <nemobox.h>
#include <nemoattr.h>
#include <showmisc.h>
#include <nemomisc.h>
#include <nemolog.h>

void __attribute__((constructor(101))) nemoshow_initialize(void)
{
	SkGraphics::Init();
}

void __attribute__((destructor(101))) nemoshow_finalize(void)
{
	SkGraphics::Term();
}

struct nemoshow *nemoshow_create(void)
{
	struct nemoshow *show;

	show = (struct nemoshow *)malloc(sizeof(struct nemoshow));
	if (show == NULL)
		return NULL;
	memset(show, 0, sizeof(struct nemoshow));

#ifdef NEMOUX_WITH_SHOWEXPR
	show->expr = nemoshow_expr_create();
	if (show->expr == NULL)
		goto err1;

	show->stable = nemoshow_expr_create_symbol_table();
	if (show->stable == NULL)
		goto err2;

	nemoshow_expr_add_symbol_table(show->expr, show->stable);
#endif

	nemolist_init(&show->one_list);
	nemolist_init(&show->dirty_list);
	nemolist_init(&show->bounds_list);
	nemolist_init(&show->canvas_list);
	nemolist_init(&show->pipeline_list);
	nemolist_init(&show->tiling_list);
	nemolist_init(&show->transition_list);
	nemolist_init(&show->transition_destroy_list);

	nemolist_init(&show->scene_destroy_listener.link);

	show->sx = 1.0f;
	show->sy = 1.0f;

	show->tilesize = NEMOSHOW_DEFAULT_TILESIZE;

	return show;

#ifdef NEMOUX_WITH_SHOWEXPR
err2:
	nemoshow_expr_destroy(show->expr);
#endif

err1:
	free(show);

	return NULL;
}

void nemoshow_destroy(struct nemoshow *show)
{
	struct showtransition *trans;

	while (nemolist_empty(&show->transition_destroy_list) == 0) {
		trans = nemolist_node0(&show->transition_destroy_list, struct showtransition, link);

		nemoshow_transition_destroy(trans, 0);
	}

	while (nemolist_empty(&show->transition_list) == 0) {
		trans = nemolist_node0(&show->transition_list, struct showtransition, link);

		nemoshow_transition_destroy(trans, 0);
	}

	nemolist_remove(&show->one_list);
	nemolist_remove(&show->dirty_list);
	nemolist_remove(&show->bounds_list);
	nemolist_remove(&show->canvas_list);
	nemolist_remove(&show->pipeline_list);
	nemolist_remove(&show->tiling_list);
	nemolist_remove(&show->transition_list);
	nemolist_remove(&show->transition_destroy_list);

	nemolist_remove(&show->scene_destroy_listener.link);

#ifdef NEMOUX_WITH_SHOWEXPR
	nemoshow_expr_destroy(show->expr);
	nemoshow_expr_destroy_symbol_table(show->stable);
#endif

	if (show->pool != NULL)
		nemopool_destroy(show->pool);

	free(show);
}

int nemoshow_prepare_threads(struct nemoshow *show, int threads)
{
	show->pool = nemopool_create(threads);

	return show->pool != NULL;
}

void nemoshow_finish_threads(struct nemoshow *show)
{
	nemopool_destroy(show->pool);

	show->pool = NULL;
}

void nemoshow_set_tilesize(struct nemoshow *show, int tilesize)
{
	show->tilesize = tilesize;
}

struct showone *nemoshow_search_one(struct nemoshow *show, const char *id)
{
	struct showone *one;

	if (id == NULL || id[0] == '\0')
		return NULL;

	nemoshow_for_each(one, show) {
		if (strcmp(one->id, id) == 0)
			return one;
	}

	return NULL;
}

#ifdef NEMOUX_WITH_SHOWEXPR
void nemoshow_update_symbol(struct nemoshow *show, const char *name, double value)
{
	nemoshow_expr_add_symbol(show->stable, name, value);
}

void nemoshow_update_expression(struct nemoshow *show)
{
	struct showone *one;
	struct showattr *attr;
	int i;

	nemoshow_for_each(one, show) {
		uint32_t dirty = 0x0;

		for (i = 0; i < one->nattrs; i++) {
			attr = one->attrs[i];

			nemoattr_setd(attr->ref,
					nemoshow_expr_dispatch_expression(show->expr, attr->text));

			dirty |= attr->dirty;
		}

		nemoshow_one_dirty(one, dirty);
	}
}

void nemoshow_update_one_expression(struct nemoshow *show, struct showone *one)
{
	struct showattr *attr;
	struct showone *child;
	uint32_t dirty = 0x0;
	int i;

	for (i = 0; i < one->nattrs; i++) {
		attr = one->attrs[i];

		nemoattr_setd(attr->ref,
				nemoshow_expr_dispatch_expression(show->expr, attr->text));

		dirty |= attr->dirty;
	}

	nemoshow_children_for_each(child, one)
		nemoshow_update_one_expression(show, child);

	nemoshow_one_dirty(one, dirty);
}

void nemoshow_update_one_expression_without_dirty(struct nemoshow *show, struct showone *one)
{
	struct showattr *attr;
	struct showone *child;
	int i;

	for (i = 0; i < one->nattrs; i++) {
		attr = one->attrs[i];

		nemoattr_setd(attr->ref,
				nemoshow_expr_dispatch_expression(show->expr, attr->text));
	}

	nemoshow_children_for_each(child, one)
		nemoshow_update_one_expression_without_dirty(show, child);
}
#endif

void nemoshow_update_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showone *one, *none;

	if (scene == NULL)
		return;

	nemolist_for_each_safe(one, none, &show->dirty_list, dirty_link) {
		nemoshow_one_update(one);
	}

	nemolist_for_each_safe(one, none, &show->bounds_list, bounds_link) {
		nemoshow_one_update_bounds(one);
	}

	show->dirty_serial = 0;
}

void nemoshow_render_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showcanvas *canvas, *ncanvas;

	if (scene == NULL)
		return;

	nemolist_for_each_safe(canvas, ncanvas, &show->canvas_list, link) {
		canvas->dispatch_redraw(show, NEMOSHOW_CANVAS_ONE(canvas));

		nemotale_node_flush(canvas->node);
		nemotale_node_filter(canvas->node);

		nemolist_remove(&canvas->link);
		nemolist_init(&canvas->link);
	}

	nemolist_for_each_safe(canvas, ncanvas, &show->pipeline_list, link) {
		canvas->dispatch_redraw(show, NEMOSHOW_CANVAS_ONE(canvas));

		nemotale_node_filter(canvas->node);

		nemolist_remove(&canvas->link);
		nemolist_init(&canvas->link);
	}
}

static void nemoshow_handle_vector_canvas_render(void *arg)
{
	struct showtask *task = (struct showtask *)arg;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(task->one);

	canvas->dispatch_redraw(task->show, task->one);
}

static void nemoshow_handle_vector_canvas_render_tiled(void *arg)
{
	struct showtask *task = (struct showtask *)arg;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(task->one);

	canvas->dispatch_redraw_tiled(task->show, task->one, task->x, task->y, task->w, task->h);
}

static void nemoshow_handle_vector_canvas_render_done(void *arg)
{
	struct showtask *task = (struct showtask *)arg;
	struct nemoshow *show = task->show;
	struct showcanvas *canvas = NEMOSHOW_CANVAS(task->one);

	if (task->tag == 0) {
		nemotale_node_flush(canvas->node);
	} else if (nemotale_has_unpack_subimage(show->tale) != 0 && nemotale_node_needs_full_upload(canvas->node) == 0) {
		nemotale_node_flush_area(canvas->node, task->x, task->y, task->w, task->h);
	}

	free(task);
}

void nemoshow_divide_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct nemopool *pool = show->pool;
	struct showcanvas *canvas, *ncanvas;
	struct showtask *task;

	if (scene == NULL || pool == NULL)
		return;

	nemolist_for_each_safe(canvas, ncanvas, &show->canvas_list, link) {
		int needs_tiling = 0;

		if (canvas->dispatch_redraw_tiled == NULL)
			continue;

		if (canvas->viewport.width >= show->tilesize || canvas->viewport.height >= show->tilesize) {
			if (nemoshow_one_has_state(NEMOSHOW_CANVAS_ONE(canvas), NEMOSHOW_REDRAW_STATE)) {
				needs_tiling = 1;
			} else {
				SkIRect box = NEMOSHOW_CANVAS_CC(canvas, damage)->getBounds();

				if (box.width() >= show->tilesize || box.height() >= show->tilesize)
					needs_tiling = 1;
			}
		}

		if (needs_tiling != 0) {
			int cw, ch;
			int tr, tc;
			int tw, th;
			int i, j;

			cw = canvas->viewport.width;
			ch = canvas->viewport.height;
			tc = cw / show->tilesize + 1;
			tr = ch / show->tilesize + 1;
			tw = ceil(cw / tc);
			th = ceil(ch / tr);

			for (i = 0; i < tr; i++) {
				for (j = 0; j < tc; j++) {
					if (nemoshow_one_has_state(NEMOSHOW_CANVAS_ONE(canvas), NEMOSHOW_REDRAW_STATE) ||
							NEMOSHOW_CANVAS_CC(canvas, damage)->intersects(SkIRect::MakeXYWH(j * tw, i * th, tw, th))) {
						task = (struct showtask *)malloc(sizeof(struct showtask));
						task->show = show;
						task->one = NEMOSHOW_CANVAS_ONE(canvas);
						task->tag = 1;
						task->x = j * tw;
						task->y = i * th;
						task->w = tw;
						task->h = th;

						nemopool_dispatch_task(pool, nemoshow_handle_vector_canvas_render_tiled, task);
					}
				}
			}
		} else {
			task = (struct showtask *)malloc(sizeof(struct showtask));
			task->show = show;
			task->one = NEMOSHOW_CANVAS_ONE(canvas);
			task->tag = 0;

			nemopool_dispatch_task(pool, nemoshow_handle_vector_canvas_render, task);
		}

		nemolist_remove(&canvas->link);
		nemolist_insert_tail(&show->tiling_list, &canvas->link);
	}

	while (nemopool_dispatch_done(pool, nemoshow_handle_vector_canvas_render_done) == 0);

	nemolist_for_each_safe(canvas, ncanvas, &show->tiling_list, link) {
		nemoshow_one_put_state(NEMOSHOW_CANVAS_ONE(canvas), NEMOSHOW_REDRAW_STATE);

		NEMOSHOW_CANVAS_CC(canvas, damage)->setEmpty();

		if (nemotale_has_unpack_subimage(show->tale) == 0 || nemotale_node_needs_full_upload(canvas->node) != 0)
			nemotale_node_flush(canvas->node);

		nemotale_node_filter(canvas->node);

		nemolist_remove(&canvas->link);
		nemolist_init(&canvas->link);
	}
}

static void nemoshow_handle_scene_destroy_signal(struct nemolistener *listener, void *data)
{
	struct nemoshow *show = (struct nemoshow *)container_of(listener, struct nemoshow, scene_destroy_listener);

	show->scene = NULL;

	nemolist_remove(&show->scene_destroy_listener.link);
	nemolist_init(&show->scene_destroy_listener.link);
}

int nemoshow_set_scene(struct nemoshow *show, struct showone *one)
{
	struct showone *child;

	if (show->scene == one)
		return 0;

	if (show->scene != NULL)
		nemoshow_put_scene(show);

	show->scene = one;

	show->scene_destroy_listener.notify = nemoshow_handle_scene_destroy_signal;
	nemosignal_add(&one->destroy_signal, &show->scene_destroy_listener);

	nemoshow_attach_ones(show, one);

	nemoshow_one_dirty(one, NEMOSHOW_SIZE_DIRTY | NEMOSHOW_CHILDREN_DIRTY);

	return 0;
}

void nemoshow_put_scene(struct nemoshow *show)
{
	if (show->scene == NULL)
		return;

	nemoshow_detach_ones(show->scene);

	show->scene = NULL;

	nemolist_remove(&show->scene_destroy_listener.link);
	nemolist_init(&show->scene_destroy_listener.link);
}

int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height)
{
	struct showone *one;
	struct showone *child;

	if (show->width == width && show->height == height)
		return 0;

	show->width = width;
	show->height = height;

	if (show->scene != NULL)
		nemoshow_one_dirty(show->scene, NEMOSHOW_VIEWPORT_DIRTY);

	return 1;
}

int nemoshow_set_scale(struct nemoshow *show, double sx, double sy)
{
	struct showone *one;
	struct showone *child;

	if (show->scene == NULL)
		return 0;

	if (show->sx == sx && show->sy == sy)
		return 0;

	show->sx = sx;
	show->sy = sy;

	nemoshow_one_dirty(show->scene, NEMOSHOW_VIEWPORT_DIRTY);

	return 1;
}

int nemoshow_contain_canvas(struct nemoshow *show, struct showone *one, float x, float y, float *sx, float *sy)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	if (canvas->contain_point != NULL) {
		nemotale_node_transform_from_global(canvas->node, x, y, sx, sy);

		return canvas->contain_point(show, one, *sx, *sy);
	}

	return nemotale_contain_node(show->tale, canvas->node, x, y, sx, sy);
}

void nemoshow_attach_one(struct nemoshow *show, struct showone *one)
{
	nemolist_insert_tail(&show->one_list, &one->link);
	one->show = show;

	nemoshow_one_dirty(one, NEMOSHOW_ALL_DIRTY);
}

void nemoshow_detach_one(struct showone *one)
{
	nemolist_remove(&one->link);
	nemolist_init(&one->link);

	one->show = NULL;
}

void nemoshow_attach_ones(struct nemoshow *show, struct showone *one)
{
	struct showone *child;
	int i;

	if (one->show == show)
		return;

	if (one->show != NULL)
		nemoshow_detach_one(one);

	nemoshow_attach_one(show, one);

	nemoshow_children_for_each(child, one) {
		nemoshow_attach_ones(show, child);
	}

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		if (one->refs[i] != NULL)
			nemoshow_attach_ones(show, one->refs[i]->src);
	}
}

void nemoshow_detach_ones(struct showone *one)
{
	struct showone *child;

	nemoshow_detach_one(one);

	nemoshow_children_for_each(child, one) {
		nemoshow_detach_ones(child);
	}
}

void nemoshow_attach_transition(struct nemoshow *show, struct showtransition *trans)
{
	int i;

	trans->serial = ++show->transition_serial;

	for (i = 0; i < trans->nsequences; i++) {
		nemoshow_sequence_prepare(trans->sequences[i], trans->serial);
	}

	nemolist_insert(&show->transition_list, &trans->link);
}

void nemoshow_detach_transition(struct nemoshow *show, struct showtransition *trans)
{
	nemolist_remove(&trans->link);
	nemolist_init(&trans->link);
}

void nemoshow_attach_transition_after(struct nemoshow *show, struct showtransition *trans, struct showtransition *ntrans)
{
	nemolist_insert_tail(&trans->children_list, &ntrans->children_link);

	ntrans->serial = ++show->transition_serial;
}

void nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs)
{
	struct showtransition *trans, *ntrans;
	struct nemolist transition_list;
	int done, i;

	if (nemoshow_has_state(show, NEMOSHOW_ONTIME_STATE))
		msecs = time_current_msecs();

	nemolist_init(&transition_list);

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		done = nemoshow_transition_dispatch(trans, msecs);
		if (done != 0) {
			if (trans->repeat == 1) {
				nemolist_insert_list(&transition_list, &trans->children_list);
				nemolist_init(&trans->children_list);

				nemolist_remove(&trans->link);
				nemolist_insert(&show->transition_destroy_list, &trans->link);
			} else if (trans->repeat == 0 || --trans->repeat) {
				trans->stime = 0;
				trans->delay = 0;

				for (i = 0; i < trans->nsequences; i++) {
					nemoshow_sequence_prepare(trans->sequences[i], trans->serial);
				}
			}
		}
	}

	nemolist_for_each_safe(trans, ntrans, &transition_list, children_link) {
		for (i = 0; i < trans->nsequences; i++) {
			nemoshow_sequence_prepare(trans->sequences[i], trans->serial);
		}

		nemolist_insert(&show->transition_list, &trans->link);

		nemolist_remove(&trans->children_link);
		nemolist_init(&trans->children_link);
	}
}

void nemoshow_destroy_transition(struct nemoshow *show)
{
	struct showtransition *trans;
	int had_transitions = nemolist_empty(&show->transition_destroy_list) == 0;

	while (nemolist_empty(&show->transition_destroy_list) == 0) {
		trans = nemolist_node0(&show->transition_destroy_list, struct showtransition, link);

		nemoshow_transition_destroy(trans, 1);
	}

	if (had_transitions != 0 && show->dispatch_done != NULL && nemolist_empty(&show->transition_list) != 0) {
		show->dispatch_done(show->dispatch_data);
	}
}

int nemoshow_has_transition(struct nemoshow *show)
{
	return !nemolist_empty(&show->transition_list);
}

void nemoshow_revoke_transition(struct nemoshow *show, struct showone *one, const char *name)
{
	struct showtransition *trans;
	struct showone *sequence;
	struct showone *frame;
	struct showone *child;
	struct showset *set;
	struct showact *act, *nact;
	struct nemoattr *attr;
	int i;

	attr = nemoobject_get(&one->object, name);
	if (attr == NULL)
		return;

	nemolist_for_each(trans, &show->transition_list, link) {
		for (i = 0; i < trans->nsequences; i++) {
			sequence = trans->sequences[i];

			nemoshow_children_for_each(frame, sequence) {
				nemoshow_children_for_each(child, frame) {
					set = NEMOSHOW_SET(child);

					if (set->src == one) {
						nemolist_for_each_safe(act, nact, &set->act_list, link) {
							if (act->attr == attr) {
								nemolist_remove(&act->link);

								free(act);
							}
						}
					}
				}
			}
		}
	}
}

void nemoshow_set_keyboard_focus(struct nemoshow *show, struct showone *one)
{
	nemotale_set_keyboard_focus(show->tale, NEMOSHOW_CANVAS_AT(one, node));
}
