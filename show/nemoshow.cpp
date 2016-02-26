#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemoshow.h>
#include <nemoxml.h>
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

	show->expr = nemoshow_expr_create();
	if (show->expr == NULL)
		goto err1;

	show->stable = nemoshow_expr_create_symbol_table();
	if (show->stable == NULL)
		goto err2;

	nemoshow_expr_add_symbol_table(show->expr, show->stable);

	nemolist_init(&show->one_list);
	nemolist_init(&show->dirty_list);
	nemolist_init(&show->transition_list);
	nemolist_init(&show->transition_destroy_list);

	nemolist_init(&show->scene_destroy_listener.link);

	show->sx = 1.0f;
	show->sy = 1.0f;

	return show;

err2:
	nemoshow_expr_destroy(show->expr);

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
	nemolist_remove(&show->transition_list);
	nemolist_remove(&show->transition_destroy_list);

	nemolist_remove(&show->scene_destroy_listener.link);

	nemoshow_expr_destroy(show->expr);
	nemoshow_expr_destroy_symbol_table(show->stable);

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

static struct showone *nemoshow_create_one(struct nemoshow *show, struct xmlnode *node)
{
	struct showone *one = NULL;

	if (strcmp(node->name, "scene") == 0) {
		one = nemoshow_scene_create();
	} else if (strcmp(node->name, "canvas") == 0) {
		one = nemoshow_canvas_create();
	} else if (strcmp(node->name, "rect") == 0) {
		one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	} else if (strcmp(node->name, "rrect") == 0) {
		one = nemoshow_item_create(NEMOSHOW_RRECT_ITEM);
	} else if (strcmp(node->name, "circle") == 0) {
		one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	} else if (strcmp(node->name, "arc") == 0) {
		one = nemoshow_item_create(NEMOSHOW_ARC_ITEM);
	} else if (strcmp(node->name, "pie") == 0) {
		one = nemoshow_item_create(NEMOSHOW_PIE_ITEM);
	} else if (strcmp(node->name, "donut") == 0) {
		one = nemoshow_item_create(NEMOSHOW_DONUT_ITEM);
	} else if (strcmp(node->name, "ring") == 0) {
		one = nemoshow_item_create(NEMOSHOW_RING_ITEM);
	} else if (strcmp(node->name, "text") == 0) {
		one = nemoshow_item_create(NEMOSHOW_TEXT_ITEM);
	} else if (strcmp(node->name, "path") == 0) {
		one = nemoshow_item_create(NEMOSHOW_PATHGROUP_ITEM);
	} else if (strcmp(node->name, "image") == 0) {
		one = nemoshow_item_create(NEMOSHOW_IMAGE_ITEM);
	} else if (strcmp(node->name, "group") == 0) {
		one = nemoshow_item_create(NEMOSHOW_GROUP_ITEM);
	} else if (strcmp(node->name, "svg") == 0) {
		one = nemoshow_item_create(NEMOSHOW_SVG_ITEM);
	} else if (strcmp(node->name, "sequence") == 0) {
		one = nemoshow_sequence_create();
	} else if (strcmp(node->name, "frame") == 0) {
		one = nemoshow_sequence_create_frame();
	} else if (strcmp(node->name, "set") == 0) {
		one = nemoshow_sequence_create_set();
	} else if (strcmp(node->name, "ease") == 0) {
		one = nemoshow_ease_create();
	} else if (strcmp(node->name, "matrix") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_MATRIX_MATRIX);
	} else if (strcmp(node->name, "scale") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_SCALE_MATRIX);
	} else if (strcmp(node->name, "rotate") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_ROTATE_MATRIX);
	} else if (strcmp(node->name, "translate") == 0) {
		one = nemoshow_matrix_create(NEMOSHOW_TRANSLATE_MATRIX);
	} else if (strcmp(node->name, "moveto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_MOVETO_PATH);
	} else if (strcmp(node->name, "lineto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_LINETO_PATH);
	} else if (strcmp(node->name, "curveto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CURVETO_PATH);
	} else if (strcmp(node->name, "close") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CLOSE_PATH);
	} else if (strcmp(node->name, "cmd") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CMD_PATH);
	} else if (strcmp(node->name, "rectto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_RECT_PATH);
	} else if (strcmp(node->name, "circleto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_CIRCLE_PATH);
	} else if (strcmp(node->name, "textto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_TEXT_PATH);
	} else if (strcmp(node->name, "svgto") == 0) {
		one = nemoshow_path_create(NEMOSHOW_SVG_PATH);
	} else if (strcmp(node->name, "blur") == 0) {
		one = nemoshow_filter_create(NEMOSHOW_BLUR_FILTER);
	} else if (strcmp(node->name, "linear") == 0) {
		one = nemoshow_shader_create(NEMOSHOW_LINEAR_GRADIENT_SHADER);
	} else if (strcmp(node->name, "radial") == 0) {
		one = nemoshow_shader_create(NEMOSHOW_RADIAL_GRADIENT_SHADER);
	} else if (strcmp(node->name, "stop") == 0) {
		one = nemoshow_stop_create();
	} else if (strcmp(node->name, "cons") == 0) {
		one = nemoshow_cons_create();
	} else if (strcmp(node->name, "font") == 0) {
		one = nemoshow_font_create();
	} else if (strcmp(node->name, "defs") == 0) {
		one = nemoshow_one_create(NEMOSHOW_DEFS_TYPE);
	}

	if (one != NULL) {
		int i;

		for (i = 0; i < node->nattrs; i++) {
			struct showprop *prop;

			prop = nemoshow_get_property(node->attrs[i*2+0]);
			if (prop == NULL)
				continue;

			if (node->attrs[i*2+1][0] == '!') {
				struct showattr *attr;

				nemoobject_seti(&one->object, node->attrs[i*2+0], 0.0f);

				attr = nemoshow_one_create_attr(
						node->attrs[i*2+0],
						node->attrs[i*2+1] + 1,
						nemoobject_get(&one->object, node->attrs[i*2+0]),
						prop->dirty);

				NEMOBOX_APPEND(one->attrs, one->sattrs, one->nattrs, attr);
			} else {
				if (prop->type == NEMOSHOW_STRING_PROP) {
					nemoobject_sets(&one->object, node->attrs[i*2+0], node->attrs[i*2+1], strlen(node->attrs[i*2+1]));
				} else if (prop->type == NEMOSHOW_DOUBLE_PROP) {
					nemoobject_setd(&one->object, node->attrs[i*2+0], strtod(node->attrs[i*2+1], NULL));
				} else if (prop->type == NEMOSHOW_INTEGER_PROP) {
					nemoobject_seti(&one->object, node->attrs[i*2+0], strtoul(node->attrs[i*2+1], NULL, 10));
				} else if (prop->type == NEMOSHOW_COLOR_PROP) {
					uint32_t c = nemoshow_color_parse(node->attrs[i*2+1]);
					char attr[NEMOSHOW_ATTR_NAME_MAX];

					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:r", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_R(c));
					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:g", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_G(c));
					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:b", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_B(c));
					snprintf(attr, NEMOSHOW_ATTR_NAME_MAX, "%s:a", node->attrs[i*2+0]);
					nemoobject_setd(&one->object, attr, (double)NEMOSHOW_COLOR_UINT32_A(c));

					nemoobject_seti(&one->object, node->attrs[i*2+0], 1);
				}
			}
		}
	}

	return one;
}

static int nemoshow_load_one(struct nemoshow *show, struct showone *one, struct xmlnode *node);
static int nemoshow_load_item(struct nemoshow *show, struct showone *item, struct xmlnode *node);
static int nemoshow_load_canvas(struct nemoshow *show, struct showone *canvas, struct xmlnode *node);
static int nemoshow_load_matrix(struct nemoshow *show, struct showone *matrix, struct xmlnode *node);
static int nemoshow_load_scene(struct nemoshow *show, struct showone *scene, struct xmlnode *node);
static int nemoshow_load_frame(struct nemoshow *show, struct showone *frame, struct xmlnode *node);
static int nemoshow_load_sequence(struct nemoshow *show, struct showone *sequence, struct xmlnode *node);
static int nemoshow_load_show(struct nemoshow *show, struct xmlnode *node);

static int nemoshow_load_one(struct nemoshow *show, struct showone *parent, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(parent, one);
		}
	}

	return 0;
}

static int nemoshow_load_item(struct nemoshow *show, struct showone *item, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(item, one);

			if (one->type == NEMOSHOW_MATRIX_TYPE) {
				nemoshow_load_matrix(show, one, child);
			} else if (one->type == NEMOSHOW_ITEM_TYPE) {
				nemoshow_load_item(show, one, child);
			}
		}
	}

	return 0;
}

static int nemoshow_load_canvas(struct nemoshow *show, struct showone *canvas, struct xmlnode *node)
{
	struct showcanvas *cone = NEMOSHOW_CANVAS(canvas);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(canvas, one);

			if (one->type == NEMOSHOW_ITEM_TYPE) {
				nemoshow_load_item(show, one, child);
			} else if (one->type == NEMOSHOW_SHADER_TYPE) {
				nemoshow_load_one(show, one, child);
			} else if (one->type == NEMOSHOW_DEFS_TYPE) {
				nemoshow_load_one(show, one, child);
			}
		}
	}

	return 0;
}

static int nemoshow_load_matrix(struct nemoshow *show, struct showone *matrix, struct xmlnode *node)
{
	struct showmatrix *mone = NEMOSHOW_MATRIX(matrix);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(matrix, one);

			if (one->sub == NEMOSHOW_MATRIX_MATRIX) {
				nemoshow_load_matrix(show, one, child);
			}
		}
	}

	return 0;
}

static int nemoshow_load_scene(struct nemoshow *show, struct showone *scene, struct xmlnode *node)
{
	struct showscene *sone = NEMOSHOW_SCENE(scene);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(scene, one);

			if (one->type == NEMOSHOW_CANVAS_TYPE) {
				nemoshow_load_canvas(show, one, child);
			} else if (one->type == NEMOSHOW_MATRIX_TYPE) {
				nemoshow_load_matrix(show, one, child);
			}
		}
	}

	return 0;
}

static int nemoshow_load_frame(struct nemoshow *show, struct showone *frame, struct xmlnode *node)
{
	struct showframe *fone = NEMOSHOW_FRAME(frame);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(frame, one);
		}
	}

	return 0;
}

static int nemoshow_load_sequence(struct nemoshow *show, struct showone *sequence, struct xmlnode *node)
{
	struct showsequence *sone = NEMOSHOW_SEQUENCE(sequence);
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);
			nemoshow_one_attach(sequence, one);

			if (one->type == NEMOSHOW_FRAME_TYPE) {
				nemoshow_load_frame(show, one, child);
			}
		}
	}

	return 0;
}

static int nemoshow_load_show(struct nemoshow *show, struct xmlnode *node)
{
	struct xmlnode *child;
	struct showone *one;

	nemolist_for_each(child, &node->children, link) {
		one = nemoshow_create_one(show, child);
		if (one != NULL) {
			nemoshow_attach_one(show, one);

			if (one->type == NEMOSHOW_SCENE_TYPE) {
				nemoshow_load_scene(show, one, child);
			}
		}
	}

	return 0;
}

int nemoshow_load_xml(struct nemoshow *show, const char *path)
{
	struct nemoxml *xml;
	struct xmlnode *node;

	xml = nemoxml_create();
	nemoxml_load_file(xml, path);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->children, link) {
		if (strcmp(node->name, "show") == 0)
			nemoshow_load_show(show, node);
	}

	nemoxml_destroy(xml);

	return 0;
}

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

void nemoshow_arrange_one(struct nemoshow *show)
{
	struct showone *one;

	nemoshow_for_each(one, show) {
		if (one->type == NEMOSHOW_EASE_TYPE) {
			nemoshow_ease_arrange(one);
		} else if (one->type == NEMOSHOW_CANVAS_TYPE) {
			nemoshow_canvas_arrange(one);
		} else if (one->type == NEMOSHOW_ITEM_TYPE) {
			nemoshow_item_arrange(one);
		} else if (one->type == NEMOSHOW_MATRIX_TYPE) {
			nemoshow_matrix_arrange(one);
		} else if (one->type == NEMOSHOW_FILTER_TYPE) {
			nemoshow_filter_arrange(one);
		} else if (one->type == NEMOSHOW_SHADER_TYPE) {
			nemoshow_shader_arrange(one);
		} else if (one->type == NEMOSHOW_FONT_TYPE) {
			nemoshow_font_arrange(one);
		} else if (one->type == NEMOSHOW_PATH_TYPE) {
			nemoshow_path_arrange(one);
		}
	}
}

void nemoshow_update_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showone *one, *none;

	if (scene == NULL)
		return;

	nemolist_for_each_safe(one, none, &show->dirty_list, dirty_link) {
		nemoshow_one_update(one);
	}

	show->dirty_serial = 0;
}

void nemoshow_render_one(struct nemoshow *show)
{
	struct showone *scene = show->scene;
	struct showcanvas *canvas;
	struct showone *one, *none;

	if (scene == NULL)
		return;

	nemolist_for_each_safe(one, none, &show->dirty_list, dirty_link) {
		nemoshow_one_update(one);
	}

	show->dirty_serial = 0;

	if (NEMOSHOW_SCENE_AT(scene, needs_resize) != 0) {
		NEMOSHOW_SCENE_AT(scene, needs_resize) = 0;

		nemotale_resize(show->tale, NEMOSHOW_SCENE_AT(scene, width), NEMOSHOW_SCENE_AT(scene, height));
	}

	nemoshow_children_for_each(one, scene) {
		if (one->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(one);

			if (canvas->needs_resize != 0) {
				canvas->needs_resize = 0;

				nemoshow_canvas_resize(one);
			}

			if (canvas->is_mapped == 0) {
				canvas->is_mapped = 1;

				nemotale_attach_node(show->tale, canvas->node);
			}

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
	struct showscene *scene;
	struct showone *child;
	struct showcanvas *canvas;

	if (show->scene == one)
		return 0;

	if (show->scene != NULL)
		nemoshow_put_scene(show);

	show->scene = one;

	show->scene_destroy_listener.notify = nemoshow_handle_scene_destroy_signal;
	nemosignal_add(&one->destroy_signal, &show->scene_destroy_listener);

	scene = NEMOSHOW_SCENE(one);

	nemoshow_children_for_each(child, one) {
		if (child->type == NEMOSHOW_CANVAS_TYPE) {
			canvas = NEMOSHOW_CANVAS(child);

			canvas->is_mapped = 0;
		}
	}

	scene->needs_resize = 1;

	nemoshow_attach_ones(show, one);

	return 0;
}

void nemoshow_put_scene(struct nemoshow *show)
{
	if (show->scene == NULL)
		return;

	nemoshow_detach_ones(show, show->scene);

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

void nemoshow_attach_canvas(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_attach_node(show->tale, canvas->node);

	canvas->show = show;
	canvas->is_mapped = 1;
}

void nemoshow_detach_canvas(struct nemoshow *show, struct showone *one)
{
	struct showcanvas *canvas = NEMOSHOW_CANVAS(one);

	nemotale_detach_node(show->tale, canvas->node);

	canvas->show = NULL;
	canvas->is_mapped = 0;
}

void nemoshow_above_canvas(struct nemoshow *show, struct showone *one, struct showone *above)
{
	nemotale_above_node(show->tale,
			NEMOSHOW_CANVAS_AT(one, node),
			NEMOSHOW_CANVAS_AT(above, node));
}

void nemoshow_below_canvas(struct nemoshow *show, struct showone *one, struct showone *below)
{
	nemotale_below_node(show->tale,
			NEMOSHOW_CANVAS_AT(one, node),
			NEMOSHOW_CANVAS_AT(below, node));
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

void nemoshow_detach_one(struct nemoshow *show, struct showone *one)
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
		nemoshow_detach_one(one->show, one);

	nemoshow_attach_one(show, one);

	nemoshow_children_for_each(child, one) {
		nemoshow_attach_ones(show, child);
	}

	for (i = 0; i < NEMOSHOW_LAST_REF; i++) {
		if (one->refs[i] != NULL)
			nemoshow_attach_ones(show, one->refs[i]->src);
	}
}

void nemoshow_detach_ones(struct nemoshow *show, struct showone *one)
{
	struct showone *child;

	if (one->show == show) {
		nemoshow_detach_one(show, one);
	}

	nemoshow_children_for_each(child, one) {
		nemoshow_detach_ones(show, child);
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
}

void nemoshow_attach_transition_after(struct nemoshow *show, struct showtransition *trans, struct showtransition *ntrans)
{
	NEMOBOX_APPEND(trans->transitions, trans->stransitions, trans->ntransitions, ntrans);

	ntrans->serial = ++show->transition_serial;
	ntrans->parent = trans;
}

void nemoshow_dispatch_transition(struct nemoshow *show, uint32_t msecs)
{
	struct showtransition *trans, *ntrans;
	struct showtransition *strans[64];
	int scount = 0;
	int done, i, j;

	nemolist_for_each_safe(trans, ntrans, &show->transition_list, link) {
		done = nemoshow_transition_dispatch(trans, msecs);
		if (done != 0) {
			if (trans->repeat == 1) {
				for (i = 0; i < trans->ntransitions; i++) {
					strans[scount++] = trans->transitions[i];
				}

				trans->ntransitions = 0;

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

	for (i = 0; i < scount; i++) {
		trans = strans[i];
		trans->parent = NULL;

		for (j = 0; j < trans->nsequences; j++) {
			nemoshow_sequence_prepare(trans->sequences[j], trans->serial);
		}

		nemolist_insert(&show->transition_list, &trans->link);
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

void nemoshow_dump_all(struct nemoshow *show, FILE *out)
{
	struct showone *one;

	nemoshow_for_each(one, show) {
		nemoshow_one_dump(one, out);
	}
}
