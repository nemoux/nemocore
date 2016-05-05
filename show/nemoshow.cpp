#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <nemobox.h>
#include <nemoattr.h>
#include <showmisc.h>
#include <nemomisc.h>
#include <skiaconfig.hpp>

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
	nemolist_init(&show->transition_list);
	nemolist_init(&show->transition_destroy_list);

	nemolist_init(&show->scene_destroy_listener.link);

	show->sx = 1.0f;
	show->sy = 1.0f;

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
	nemolist_remove(&show->transition_list);
	nemolist_remove(&show->transition_destroy_list);

	nemolist_remove(&show->scene_destroy_listener.link);

#ifdef NEMOUX_WITH_SHOWEXPR
	nemoshow_expr_destroy(show->expr);
	nemoshow_expr_destroy_symbol_table(show->stable);
#endif

	free(show);
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
	struct showcanvas *canvas;
	struct showone *one, *none;

	nemolist_for_each_safe(one, none, &show->dirty_list, dirty_link) {
		nemoshow_one_update(one);
	}

	nemolist_for_each_safe(one, none, &show->bounds_list, bounds_link) {
		nemoshow_one_update_bounds(one);
	}

	show->dirty_serial = 0;

	nemoshow_children_for_each(one, scene) {
		if (one->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(one);

			if (canvas->needs_resize != 0) {
				canvas->needs_resize = 0;

				nemoshow_canvas_resize(one);
			}

			if (canvas->viewport.dirty != 0) {
				canvas->viewport.dirty = 0;

				nemoshow_canvas_set_viewport(show, one,
						(double)show->width / (double)NEMOSHOW_SCENE_AT(scene, width) * show->sx,
						(double)show->height / (double)NEMOSHOW_SCENE_AT(scene, height) * show->sy);

				if (canvas->dispatch_resize != NULL)
					canvas->dispatch_resize(show, one, canvas->viewport.width, canvas->viewport.height);
			}
		}
	}
}

void nemoshow_render_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showcanvas *canvas, *ncanvas;
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

	nemolist_for_each_safe(canvas, ncanvas, &show->canvas_list, link) {
		one = NEMOSHOW_CANVAS_ONE(canvas);

		if (canvas->needs_redraw != 0) {
			canvas->needs_redraw = 0;

			if (canvas->dispatch_redraw != NULL)
				canvas->dispatch_redraw(show, one);
			else
				nemoshow_canvas_redraw_one(show, one);
		}

		if (one->sub == NEMOSHOW_CANVAS_VECTOR_TYPE) {
			nemotale_node_flush_gl(show->tale, canvas->node);
			nemotale_node_filter_gl(show->tale, canvas->node);
		} else if (one->sub == NEMOSHOW_CANVAS_PIXMAN_TYPE) {
			nemotale_node_flush_gl(show->tale, canvas->node);
			nemotale_node_filter_gl(show->tale, canvas->node);
		} else if (one->sub == NEMOSHOW_CANVAS_OPENGL_TYPE || one->sub == NEMOSHOW_CANVAS_PIPELINE_TYPE) {
			nemotale_node_filter_gl(show->tale, canvas->node);
		}

		nemolist_remove(&canvas->link);
		nemolist_init(&canvas->link);
	}

	nemoshow_children_for_each(one, scene) {
		if (one->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(one);

			if (canvas->needs_resize != 0) {
				canvas->needs_resize = 0;

				nemoshow_canvas_resize(one);
			}

			if (nemotale_node_is_mapped(canvas->node) == 0)
				nemotale_attach_node(show->tale, canvas->node);

			if (canvas->viewport.dirty != 0) {
				canvas->viewport.dirty = 0;

				nemoshow_canvas_set_viewport(show, one,
						(double)show->width / (double)NEMOSHOW_SCENE_AT(scene, width) * show->sx,
						(double)show->height / (double)NEMOSHOW_SCENE_AT(scene, height) * show->sy);

				if (canvas->dispatch_resize != NULL)
					canvas->dispatch_resize(show, one, canvas->viewport.width, canvas->viewport.height);
			}

			if (canvas->needs_redraw != 0) {
				canvas->needs_redraw = 0;

				if (canvas->dispatch_redraw != NULL)
					canvas->dispatch_redraw(show, one);
				else
					nemoshow_canvas_redraw_one(show, one);
			}
		}
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

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_CANVAS_TYPE)
			nemotale_detach_node(NEMOSHOW_CANVAS_AT(child, node));
	}

	nemoshow_attach_ones(show, one);

	nemotale_resize(show->tale, NEMOSHOW_SCENE_AT(one, width), NEMOSHOW_SCENE_AT(one, height));

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

	nemotale_clear_node(show->tale);
}

int nemoshow_set_size(struct nemoshow *show, uint32_t width, uint32_t height)
{
	struct showone *one;
	struct showone *child;

	if (show->width == width && show->height == height)
		return 0;

	nemotale_set_viewport(show->tale, width, height);

	show->width = width;
	show->height = height;

	if (show->scene != NULL) {
		one = show->scene;

		nemoshow_children_for_each(child, one) {
			if (child->type == NEMOSHOW_CANVAS_TYPE) {
				NEMOSHOW_CANVAS_AT(child, viewport.dirty) = 1;
			}
		}
	}

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

	one = show->scene;

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_CANVAS_TYPE) {
			NEMOSHOW_CANVAS_AT(child, viewport.dirty) = 1;
		}
	}

	return 1;
}

void nemoshow_above_canvas(struct nemoshow *show, struct showone *one, struct showone *above)
{
	nemolist_remove(&one->children_link);

	if (above != NULL) {
		nemotale_above_node(show->tale,
				NEMOSHOW_CANVAS_AT(one, node),
				NEMOSHOW_CANVAS_AT(above, node));

		nemolist_insert_tail(&above->children_link, &one->children_link);
	} else {
		nemotale_above_node(show->tale,
				NEMOSHOW_CANVAS_AT(one, node),
				NULL);

		nemolist_insert_tail(&one->parent->children_list, &one->children_link);
	}
}

void nemoshow_below_canvas(struct nemoshow *show, struct showone *one, struct showone *below)
{
	nemolist_remove(&one->children_link);

	if (below != NULL) {
		nemotale_below_node(show->tale,
				NEMOSHOW_CANVAS_AT(one, node),
				NEMOSHOW_CANVAS_AT(below, node));

		nemolist_insert(&below->children_link, &one->children_link);
	} else {
		nemotale_below_node(show->tale,
				NEMOSHOW_CANVAS_AT(one, node),
				NULL);

		nemolist_insert(&one->parent->children_list, &one->children_link);
	}
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

void nemoshow_damage_canvas_all(struct nemoshow *show)
{
	struct showone *one;
	struct showone *child;

	if (show->scene == NULL)
		return;

	one = show->scene;

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_CANVAS_TYPE) {
			nemotale_node_damage_all(NEMOSHOW_CANVAS_AT(child, node));
		}
	}
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

	if (one->type == NEMOSHOW_CANVAS_TYPE)
		nemotale_detach_node(NEMOSHOW_CANVAS_AT(one, node));
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

void nemoshow_set_keyboard_focus(struct nemoshow *show, struct showone *one)
{
	nemotale_set_keyboard_focus(show->tale, NEMOSHOW_CANVAS_AT(one, node));
}

void nemoshow_dump_all(struct nemoshow *show, FILE *out)
{
	struct showone *one;

	nemoshow_for_each(one, show) {
		nemoshow_one_dump(one, out);
	}
}
