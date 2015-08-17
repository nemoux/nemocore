#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <svghelper.h>
#include <nemoshow.h>
#include <showsvg.h>
#include <showsvg.hpp>
#include <stringhelper.h>
#include <nemomisc.h>

static double nemoshow_svg_get_length(struct svgcontext *context, const char *svalue, int orientation, const char *dvalue)
{
	const char *units = NULL;
	const char *value = svalue != NULL ? svalue : dvalue;
	double length;

	length = string_parse_float_with_endptr(value, 0, strlen(value), &units);
	if (units == NULL) {
		return length;
	} else if (units[0] == 'p' && units[1] == 'x') {
		return length;
	} else if (units[0] == 'p' && units[1] == 't') {
		return length / 72.0f * NEMOSHOW_SVG_DEFAULT_DPI;
	} else if (units[0] == 'i' && units[1] == 'n') {
		return length * NEMOSHOW_SVG_DEFAULT_DPI;
	} else if (units[0] == 'c' && units[1] == 'm') {
		return length / 2.54f * NEMOSHOW_SVG_DEFAULT_DPI;
	} else if (units[0] == 'm' && units[1] == 'm') {
		return length / 25.4f * NEMOSHOW_SVG_DEFAULT_DPI;
	} else if (units[0] == 'p' && units[1] == 'c') {
		return length / 6.0f * NEMOSHOW_SVG_DEFAULT_DPI;
	} else if (units[0] == 'e' && units[1] == 'm') {
		return length * context->fontsize;
	} else if (units[0] == 'e' && units[1] == 'x') {
		return length * context->fontsize / 2.0f;
	} else if (units[0] == '%') {
		double width, height;

		if (context->viewport.is_bbox != 0) {
			width = 1.0f;
			height = 1.0f;
		} else {
			width = context->viewport.width;
			height = context->viewport.height;
		}

		if (orientation == NEMOSHOW_SVG_ORIENTATION_HORIZONTAL) {
			return length / 100.0f * width;
		} else if (orientation == NEMOSHOW_SVG_ORIENTATION_VERTICAL) {
			return length / 100.0f * height;
		} else {
			return length / 100.0f * sqrtf(pow(width, 2) + pow(height, 2)) * sqrtf(2);
		}
	}

	return length;
}

static inline void nemoshow_svg_get_viewbox_transform(struct svgcontext *context, struct xmlnode *node)
{
	struct nemotoken *token;
	double lx, ly, lw, lh;
	double sw = context->svg->width;
	double sh = context->svg->height;
	double pw = context->width;
	double ph = context->height;
	double var, par;
	int aspect_ratio, meet_slice;
	const char *value;

	NEMOSHOW_SVG_CC(context->svg, viewbox)->setIdentity();
	NEMOSHOW_SVG_CC(context->svg, viewbox)->postScale(sw / pw, sh / ph);

	value = nemoxml_node_get_attr(node, "viewBox");
	if (value != NULL) {
		token = nemotoken_create(value, strlen(value));
		nemotoken_divide(token, ' ');
		nemotoken_divide(token, ',');
		nemotoken_update(token);

		if (nemotoken_get_token_count(token) == 4) {
			lx = strtod(nemotoken_get_token(token, 0), NULL);
			ly = strtod(nemotoken_get_token(token, 1), NULL);
			lw = strtod(nemotoken_get_token(token, 2), NULL);
			lh = strtod(nemotoken_get_token(token, 3), NULL);
		}

		nemotoken_destroy(token);

		value = nemoxml_node_get_attr(node, "preserveAspectRatio");
		if (value == NULL) {
			aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_UNKNOWN;
			meet_slice = NEMOSHOW_SVG_MEETSLICE_UNKNOWN;
		} else {
			token = nemotoken_create(value, strlen(value));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, ',');
			nemotoken_update(token);

			if (nemotoken_get_token_count(token) == 2) {
				const char *svalue;

				svalue = nemotoken_get_token(token, 0);

				if (strcmp(svalue, "xMinYMin") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMINYMIN;
				else if (strcmp(svalue, "xMidYMin") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMIN;
				else if (strcmp(svalue, "xMaxYMin") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMIN;
				else if (strcmp(svalue, "xMinYMid") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMINYMID;
				else if (strcmp(svalue, "xMidYMid") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMID;
				else if (strcmp(svalue, "xMaxYMid") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMID;
				else if (strcmp(svalue, "xMinYMax") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMINYMAX;
				else if (strcmp(svalue, "xMidYMax") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMAX;
				else if (strcmp(svalue, "xMaxYMax") == 0)
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMAX;
				else
					aspect_ratio = NEMOSHOW_SVG_ASPECT_RATIO_NONE;

				svalue = nemotoken_get_token(token, 1);

				if (strcmp(svalue, "meet") == 0)
					meet_slice = NEMOSHOW_SVG_MEETSLICE_MEET;
				else if (strcmp(svalue, "slice") == 0)
					meet_slice = NEMOSHOW_SVG_MEETSLICE_SLICE;
			}

			nemotoken_destroy(token);
		}

		var = lw / lh;
		par = pw / ph;

		if (aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_NONE) {
			NEMOSHOW_SVG_CC(context->svg, viewbox)->postScale(pw / lw, ph / lh);
			NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx, -ly);
		} else if((var < par && meet_slice == NEMOSHOW_SVG_MEETSLICE_MEET) ||
				(var >= par && meet_slice == NEMOSHOW_SVG_MEETSLICE_SLICE)) {
			NEMOSHOW_SVG_CC(context->svg, viewbox)->postScale(ph / lh, ph / lh);

			if (aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMINYMIN ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMINYMID ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMINYMAX) {
				NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx, -ly);
			} else if (aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMIN ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMID ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMAX) {
				NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx - (lw - pw * lh / ph) / 2.0f, -ly);
			} else {
				NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx - (lw - pw * lh / ph), -ly);
			}
		} else {
			NEMOSHOW_SVG_CC(context->svg, viewbox)->postScale(pw / lw, pw / lw);

			if (aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMINYMIN ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMIN ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMIN) {
				NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx, -ly);
			} else if (aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMINYMID ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMID ||
					aspect_ratio == NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMID) {
				NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx, -ly - (lh - ph * lw / pw) / 2.0f);
			} else {
				NEMOSHOW_SVG_CC(context->svg, viewbox)->postTranslate(-lx, -ly - (lh - ph * lw / pw));
			}
		}
	}
}

static inline int nemoshow_svg_load_rect(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	const char *value;

	one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	nemoshow_attach_one(context->show, context->one, one);

	value = nemoxml_node_get_attr(node, "id");
	if (value != NULL) {
		strncpy(one->id, value, NEMOSHOW_ID_MAX);
	}

	value = nemoxml_node_get_attr(node, "x");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, x) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	}
	value = nemoxml_node_get_attr(node, "y");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, y) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	}
	value = nemoxml_node_get_attr(node, "width");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, width) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "100%");
	}
	value = nemoxml_node_get_attr(node, "height");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, height) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_VERTICAL, "100%");
	}

	nemoshow_item_set_fill_color(one, 255, 0, 0, 255);

	return 0;
}

static inline int nemoshow_svg_load_circle(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	const char *value;

	one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_attach_one(context->show, context->one, one);

	value = nemoxml_node_get_attr(node, "id");
	if (value != NULL) {
		strncpy(one->id, value, NEMOSHOW_ID_MAX);
	}

	value = nemoxml_node_get_attr(node, "cx");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, x) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	}
	value = nemoxml_node_get_attr(node, "cy");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, y) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	}
	value = nemoxml_node_get_attr(node, "r");
	if (value != NULL) {
		NEMOSHOW_ITEM_AT(one, r) = nemoshow_svg_get_length(context, value, NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	}

	nemoshow_item_set_stroke_color(one, 255, 255, 0, 255);

	return 0;
}

static int nemoshow_svg_load_svg(struct svgcontext *context, struct xmlnode *node)
{
	struct xmlnode *child;

	context->width = nemoshow_svg_get_length(context,
			nemoxml_node_get_attr(node, "width"),
			NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "100%");

	context->height = nemoshow_svg_get_length(context,
			nemoxml_node_get_attr(node, "height"),
			NEMOSHOW_SVG_ORIENTATION_VERTICAL, "100%");

	nemoshow_svg_get_viewbox_transform(context, node);

	nemolist_for_each(child, &node->children, link) {
		if (strcmp(child->name, "rect") == 0) {
			nemoshow_svg_load_rect(context, child);
		} else if (strcmp(child->name, "circle") == 0) {
			nemoshow_svg_load_circle(context, child);
		}
	}

	return 0;
}

int nemoshow_svg_load_uri(struct nemoshow *show, struct showone *one, const char *uri)
{
	struct svgcontext _context;
	struct svgcontext *context = &_context;
	struct nemoxml *xml;
	struct xmlnode *node;

	if (uri == NULL)
		return -1;

	xml = nemoxml_create();
	nemoxml_load_file(xml, uri);
	nemoxml_update(xml);

	context->show = show;
	context->svg = NEMOSHOW_SVG(one);
	context->one = one;
	context->xml = xml;

	nemolist_for_each(node, &xml->children, link) {
		if (strcmp(node->name, "svg") == 0) {
			nemoshow_svg_load_svg(context, node);

			break;
		}
	}

	nemoxml_destroy(xml);

	return 0;
}
