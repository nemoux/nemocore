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
#include <showitem.h>
#include <showitem.hpp>
#include <stringhelper.h>
#include <nemomisc.h>

#define NEMOSHOW_SVG_COLOR_NAME_MAX			(32)

static int nemoshow_svg_compare_color(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

static inline const uint8_t *nemoshow_svg_get_reserved_color(const char *name)
{
	static struct colormap {
		char name[NEMOSHOW_SVG_COLOR_NAME_MAX];

		uint8_t rgb[3];
	} maps[] = {
		{ "aliceblue",            { 240,248,255 } },
		{ "antiquewhite",         { 250,235,215 } },
		{ "aqua",                 {   0,255,255 } },
		{ "aquamarine",           { 127,255,212 } },
		{ "azure",                { 240,255,255 } },
		{ "beige",                { 245,245,220 } },
		{ "bisque",               { 255,228,196 } },
		{ "black",                {   0,  0,  0 } },
		{ "blanchedalmond",       { 255,235,205 } },
		{ "blue",                 {   0,  0,255 } },
		{ "blueviolet",           { 138, 43,226 } },
		{ "brown",                { 165, 42, 42 } },
		{ "burlywood",            { 222,184,135 } },
		{ "cadetblue",            {  95,158,160 } },
		{ "chartreuse",           { 127,255,  0 } },
		{ "chocolate",            { 210,105, 30 } },
		{ "coral",                { 255,127, 80 } },
		{ "cornflowerblue",       { 100,149,237 } },
		{ "cornsilk",             { 255,248,220 } },
		{ "crimson",              { 220, 20, 60 } },
		{ "cyan",                 {   0,255,255 } },
		{ "darkblue",             {   0,  0,139 } },
		{ "darkcyan",             {   0,139,139 } },
		{ "darkgoldenrod",        { 184,132, 11 } },
		{ "darkgray",             { 169,169,168 } },
		{ "darkgreen",            {   0,100,  0 } },
		{ "darkgrey",             { 169,169,169 } },
		{ "darkkhaki",            { 189,183,107 } },
		{ "darkmagenta",          { 139,  0,139 } },
		{ "darkolivegreen",       {  85,107, 47 } },
		{ "darkorange",           { 255,140,  0 } },
		{ "darkorchid",           { 153, 50,204 } },
		{ "darkred",              { 139,  0,  0 } },
		{ "darksalmon",           { 233,150,122 } },
		{ "darkseagreen",         { 143,188,143 } },
		{ "darkslateblue",        {  72, 61,139 } },
		{ "darkslategray",        {  47, 79, 79 } },
		{ "darkslategrey",        {  47, 79, 79 } },
		{ "darkturquoise",        {   0,206,209 } },
		{ "darkviolet",           { 148,  0,211 } },
		{ "deeppink",             { 255, 20,147 } },
		{ "deepskyblue",          {   0,191,255 } },
		{ "dimgray",              { 105,105,105 } },
		{ "dimgrey",              { 105,105,105 } },
		{ "dodgerblue",           {  30,144,255 } },
		{ "firebrick",            { 178, 34, 34 } },
		{ "floralwhite" ,         { 255,255,240 } },
		{ "forestgreen",          {  34,139, 34 } },
		{ "fuchsia",              { 255,  0,255 } },
		{ "gainsboro",            { 220,220,220 } },
		{ "ghostwhite",           { 248,248,255 } },
		{ "gold",                 { 215,215,  0 } },
		{ "goldenrod",            { 218,165, 32 } },
		{ "gray",                 { 128,128,128 } },
		{ "grey",                 { 128,128,128 } },
		{ "green",                {   0,128,  0 } },
		{ "greenyellow",          { 173,255, 47 } },
		{ "honeydew",             { 240,255,240 } },
		{ "hotpink",              { 255,105,180 } },
		{ "indianred",            { 205, 92, 92 } },
		{ "indigo",               {  75,  0,130 } },
		{ "ivory",                { 255,255,240 } },
		{ "khaki",                { 240,230,140 } },
		{ "lavender",             { 230,230,250 } },
		{ "lavenderblush",        { 255,240,245 } },
		{ "lawngreen",            { 124,252,  0 } },
		{ "lemonchiffon",         { 255,250,205 } },
		{ "lightblue",            { 173,216,230 } },
		{ "lightcoral",           { 240,128,128 } },
		{ "lightcyan",            { 224,255,255 } },
		{ "lightgoldenrodyellow", { 250,250,210 } },
		{ "lightgray",            { 211,211,211 } },
		{ "lightgreen",           { 144,238,144 } },
		{ "lightgrey",            { 211,211,211 } },
		{ "lightpink",            { 255,182,193 } },
		{ "lightsalmon",          { 255,160,122 } },
		{ "lightseagreen",        {  32,178,170 } },
		{ "lightskyblue",         { 135,206,250 } },
		{ "lightslategray",       { 119,136,153 } },
		{ "lightslategrey",       { 119,136,153 } },
		{ "lightsteelblue",       { 176,196,222 } },
		{ "lightyellow",          { 255,255,224 } },
		{ "lime",                 {   0,255,  0 } },
		{ "limegreen",            {  50,205, 50 } },
		{ "linen",                { 250,240,230 } },
		{ "magenta",              { 255,  0,255 } },
		{ "maroon",               { 128,  0,  0 } },
		{ "mediumaquamarine",     { 102,205,170 } },
		{ "mediumblue",           {   0,  0,205 } },
		{ "mediumorchid",         { 186, 85,211 } },
		{ "mediumpurple",         { 147,112,219 } },
		{ "mediumseagreen",       {  60,179,113 } },
		{ "mediumslateblue",      { 123,104,238 } },
		{ "mediumspringgreen",    {   0,250,154 } },
		{ "mediumturquoise",      {  72,209,204 } },
		{ "mediumvioletred",      { 199, 21,133 } },
		{ "mediumnightblue",      {  25, 25,112 } },
		{ "mintcream",            { 245,255,250 } },
		{ "mintyrose",            { 255,228,225 } },
		{ "moccasin",             { 255,228,181 } },
		{ "navajowhite",          { 255,222,173 } },
		{ "navy",                 {   0,  0,128 } },
		{ "oldlace",              { 253,245,230 } },
		{ "olive",                { 128,128,  0 } },
		{ "olivedrab",            { 107,142, 35 } },
		{ "orange",               { 255,165,  0 } },
		{ "orangered",            { 255, 69,  0 } },
		{ "orchid",               { 218,112,214 } },
		{ "palegoldenrod",        { 238,232,170 } },
		{ "palegreen",            { 152,251,152 } },
		{ "paleturquoise",        { 175,238,238 } },
		{ "palevioletred",        { 219,112,147 } },
		{ "papayawhip",           { 255,239,213 } },
		{ "peachpuff",            { 255,218,185 } },
		{ "peru",                 { 205,133, 63 } },
		{ "pink",                 { 255,192,203 } },
		{ "plum",                 { 221,160,203 } },
		{ "powderblue",           { 176,224,230 } },
		{ "purple",               { 128,  0,128 } },
		{ "red",                  { 255,  0,  0 } },
		{ "rosybrown",            { 188,143,143 } },
		{ "royalblue",            {  65,105,225 } },
		{ "saddlebrown",          { 139, 69, 19 } },
		{ "salmon",               { 250,128,114 } },
		{ "sandybrown",           { 244,164, 96 } },
		{ "seagreen",             {  46,139, 87 } },
		{ "seashell",             { 255,245,238 } },
		{ "sienna",               { 160, 82, 45 } },
		{ "silver",               { 192,192,192 } },
		{ "skyblue",              { 135,206,235 } },
		{ "slateblue",            { 106, 90,205 } },
		{ "slategray",            { 112,128,144 } },
		{ "slategrey",            { 112,128,114 } },
		{ "snow",                 { 255,255,250 } },
		{ "springgreen",          {   0,255,127 } },
		{ "steelblue",            {  70,130,180 } },
		{ "tan",                  { 210,180,140 } },
		{ "teal",                 {   0,128,128 } },
		{ "thistle",              { 216,191,216 } },
		{ "tomato",               { 255, 99, 71 } },
		{ "turquoise",            {  64,224,208 } },
		{ "violet",               { 238,130,238 } },
		{ "wheat",                { 245,222,179 } },
		{ "white",                { 255,255,255 } },
		{ "whitesmoke",           { 245,245,245 } },
		{ "yellow",               { 255,255,  0 } },
		{ "yellowgreen",          { 154,205, 50 } }
	}, *map;

	map = static_cast<struct colormap *>(bsearch(name, static_cast<void *>(maps), sizeof(maps) / sizeof(maps[0]), sizeof(maps[0]), nemoshow_svg_compare_color));
	if (map != NULL)
		return map->rgb;

	return NULL;
}

int nemoshow_svg_get_color(struct svgcontext *context, double *rgba, const char *value)
{
	if (value == NULL || value[0] == '\0')
		return -1;

	if (strcmp(value, "inherit") == 0 ||
			strcmp(value, "currentColor") == 0) {
		rgba[0] = context->rgba[0];
		rgba[1] = context->rgba[1];
		rgba[2] = context->rgba[2];
		rgba[3] = context->rgba[3];
	} else if (value[0] == '#') {
		if (strlen(value) >= 7) {
			rgba[0] = (double)string_parse_hexadecimal(value, 1, 2);
			rgba[1] = (double)string_parse_hexadecimal(value, 3, 2);
			rgba[2] = (double)string_parse_hexadecimal(value, 5, 2);
			rgba[3] = 255.0f;
		} else if (strlen(value) >= 4) {
			rgba[0] = (double)string_parse_hexadecimal(value, 1, 1);
			rgba[1] = (double)string_parse_hexadecimal(value, 2, 1);
			rgba[2] = (double)string_parse_hexadecimal(value, 3, 1);
			rgba[3] = 255.0f;
		}
	} else if (strcasestr(value, "rgb") != NULL) {
		struct nemotoken *token;

		token = nemotoken_create(value, strlen(value));
		if (token == NULL)
			return -1;
		nemotoken_divide(token, ' ');
		nemotoken_divide(token, '(');
		nemotoken_divide(token, ')');
		nemotoken_divide(token, ',');
		nemotoken_update(token);

		if (nemotoken_get_token_count(token) >= 4) {
			rgba[0] = (double)strtoul(nemotoken_get_token(token, 1), NULL, 10);
			rgba[1] = (double)strtoul(nemotoken_get_token(token, 2), NULL, 10);
			rgba[2] = (double)strtoul(nemotoken_get_token(token, 3), NULL, 10);
			rgba[3] = 255.0f;
		}

		nemotoken_destroy(token);
	} else {
		const uint8_t *rgb;

		rgb = nemoshow_svg_get_reserved_color(value);
		if (rgb != NULL) {
			rgba[0] = (double)rgb[0];
			rgba[1] = (double)rgb[1];
			rgba[2] = (double)rgb[2];
			rgba[3] = 255.0f;
		}
	}

	return 0;
}

static int nemoshow_svg_get_transform_args(struct nemotoken *token, int offset, double *args)
{
	const char *value;
	int i;

	for (i = 0; ; i++) {
		value = nemotoken_get_token(token, i + offset);
		if (value == NULL || string_is_number(value, 0, strlen(value)) == 0)
			break;

		args[i] = strtod(value, NULL);
	}

	return i;
}

int nemoshow_svg_get_transform(SkMatrix *matrix, const char *value)
{
	struct nemotoken *token;
	const char *type;
	double args[8], nargs;
	int i, count;

	token = nemotoken_create(value, strlen(value));
	if (token == NULL)
		return -1;
	nemotoken_divide(token, ' ');
	nemotoken_divide(token, '(');
	nemotoken_divide(token, ')');
	nemotoken_divide(token, ',');
	nemotoken_divide(token, '\t');
	nemotoken_update(token);

	count = nemotoken_get_token_count(token);
	i = 0;

	while (i < count) {
		type = nemotoken_get_token(token, i++);
		if (type != NULL) {
			if (strcmp(type, "translate") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->setTranslate(args[0], 0.0f);
				} else if (nargs == 2) {
					matrix->setTranslate(args[0], args[1]);
				}

				i += nargs;
			} else if (strcmp(type, "rotate") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->setRotate(args[0]);
				} else if (nargs == 3) {
					matrix->setTranslate(args[1], args[2]);
					matrix->postRotate(args[0]);
					matrix->postTranslate(-args[1], -args[2]);
				}

				i += nargs;
			} else if (strcmp(type, "scale") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->setScale(args[0], 0.0f);
				} else if (nargs == 2) {
					matrix->setScale(args[0], args[1]);
				}

				i += nargs;
			} else if (strcmp(type, "skewX") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->setSkewX(args[0]);
				}

				i += nargs;
			} else if (strcmp(type, "skewY") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 1) {
					matrix->setSkewY(args[0]);
				}

				i += nargs;
			} else if (strcmp(type, "matrix") == 0) {
				nargs = nemoshow_svg_get_transform_args(token, i, args);
				if (nargs == 6) {
					SkScalar targs[9] = {
						SkDoubleToScalar(args[0]), SkDoubleToScalar(args[1]), 0.0f,
						SkDoubleToScalar(args[2]), SkDoubleToScalar(args[3]), 0.0f,
						SkDoubleToScalar(args[4]), SkDoubleToScalar(args[5]), 1.0f
					};

					matrix->set9(targs);
				}

				i += nargs;
			}
		}
	}

	nemotoken_destroy(token);

	return 0;
}

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

static inline void nemoshow_svg_set_style(struct svgcontext *context, struct showone *one, const char *attr, const char *value)
{
	double rgba[4];

	if (strcmp(attr, "color") == 0) {
		nemoshow_svg_get_color(context, context->rgba, value);
	} else if (strcmp(attr, "fill") == 0) {
		if (strcmp(value, "none") == 0) {
		} else if (strncmp(value, "url", 3) == 0) {
			struct nemotoken *token;
			const char *url;

			token = nemotoken_create(value, strlen(value));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, '(');
			nemotoken_divide(token, ')');
			nemotoken_update(token);

			url = nemotoken_get_token(token, 1);
			if (url != NULL) {
				struct showone *shader;

				shader = nemoshow_search_one(context->show, url);
				if (shader != NULL) {
					nemoshow_item_set_shader(one, shader);
				}
			}

			nemotoken_destroy(token);
		} else {
			nemoshow_svg_get_color(context, rgba, value);

			nemoshow_item_set_fill_color(one, rgba[0], rgba[1], rgba[2], rgba[3]);
		}
	} else if (strcmp(attr, "stroke") == 0) {
		if (strcmp(value, "none") == 0) {
		} else if (strncmp(value, "url", 3) == 0) {
			struct nemotoken *token;
			const char *url;

			token = nemotoken_create(value, strlen(value));
			nemotoken_divide(token, ' ');
			nemotoken_divide(token, '(');
			nemotoken_divide(token, ')');
			nemotoken_update(token);

			url = nemotoken_get_token(token, 1);
			if (url != NULL) {
				struct showone *shader;

				shader = nemoshow_search_one(context->show, url);
				if (shader != NULL) {
					nemoshow_item_set_shader(one, shader);
				}
			}

			nemotoken_destroy(token);
		} else {
			nemoshow_svg_get_color(context, rgba, value);

			nemoshow_item_set_stroke_color(one, rgba[0], rgba[1], rgba[2], rgba[3]);
		}
	} else if (strcmp(attr, "stroke-width") == 0) {
		nemoshow_item_set_stroke_width(one, strtod(value, NULL));
	} else if (strcmp(attr, "style") == 0) {
		struct nemotoken *token;
		int i;

		token = nemotoken_create(value, strlen(value));
		nemotoken_divide(token, ' ');
		nemotoken_divide(token, ':');
		nemotoken_divide(token, ';');
		nemotoken_update(token);

		for (i = 0; i < nemotoken_get_token_count(token); i += 2) {
			nemoshow_svg_set_style(context, one,
					nemotoken_get_token(token, i+0),
					nemotoken_get_token(token, i+1));
		}

		nemotoken_destroy(token);
	} else if (strcmp(attr, "transform") == 0) {
		nemoshow_svg_get_transform(NEMOSHOW_ITEM_CC(NEMOSHOW_ITEM(one), matrix), value);
	} else if (strcmp(attr, "offset") == 0) {
		nemoshow_stop_set_offset(one, strtod(value, NULL));
	} else if (strcmp(attr, "stop-color") == 0) {
		nemoshow_svg_get_color(context, rgba, value);

		nemoshow_stop_set_fill_color(one, rgba[0], rgba[1], rgba[2], rgba[3]);
	} else if (strcmp(attr, "stop-opacity") == 0) {
		nemoshow_stop_set_offset(one, strtod(value, NULL));
	}
}

static inline void nemoshow_svg_load_style(struct svgcontext *context, struct xmlnode *node, struct showone *one)
{
	int i;

	for (i = 0; i < node->nattrs; i++) {
		nemoshow_svg_set_style(context, one,
				node->attrs[i*2+0],
				node->attrs[i*2+1]);
	}
}

static inline int nemoshow_svg_load_rect(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_circle(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_path(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_linear_gradient(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_radial_gradient(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_stop(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_group(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_defs(struct svgcontext *context, struct xmlnode *node);
static inline int nemoshow_svg_load_one(struct svgcontext *context, struct xmlnode *node);

static inline int nemoshow_svg_load_rect(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	const char *value;

	one = nemoshow_item_create(NEMOSHOW_RECT_ITEM);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	NEMOSHOW_ITEM_AT(one, x) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "x"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	NEMOSHOW_ITEM_AT(one, y) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "y"), NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	NEMOSHOW_ITEM_AT(one, width) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "width"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "100%");
	NEMOSHOW_ITEM_AT(one, height) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "height"), NEMOSHOW_SVG_ORIENTATION_VERTICAL, "100%");

	nemoshow_svg_load_style(context, node, one);

	return 0;
}

static inline int nemoshow_svg_load_circle(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	const char *value;

	one = nemoshow_item_create(NEMOSHOW_CIRCLE_ITEM);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	NEMOSHOW_ITEM_AT(one, x) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "cx"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	NEMOSHOW_ITEM_AT(one, y) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "cy"), NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	NEMOSHOW_ITEM_AT(one, r) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "r"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");

	nemoshow_svg_load_style(context, node, one);

	return 0;
}

static inline int nemoshow_svg_load_path(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	struct showone *cmd;
	const char *value;
	const char *d;

	one = nemoshow_item_create(NEMOSHOW_PATH_ITEM);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	d = nemoxml_node_get_attr(node, "d");

	cmd = nemoshow_path_create(NEMOSHOW_CMD_PATH);
	nemoobject_sets(&cmd->object, "d", d, strlen(d));
	nemoshow_attach_one(context->show, one, cmd);

	nemoshow_svg_load_style(context, node, one);

	return 0;
}

static inline int nemoshow_svg_load_linear_gradient(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	struct showone *pone;
	struct xmlnode *child;
	const char *value;
	const char *href;

	one = nemoshow_shader_create(NEMOSHOW_LINEAR_GRADIENT_SHADER);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	href = nemoxml_node_get_attr(node, "xlink:href");
	if (href != NULL)
		NEMOSHOW_SHADER_AT(one, ref) = nemoshow_search_one(context->show, href + 1);

	value = nemoxml_node_get_attr(node, "gradientUnits");
	if (value != NULL && strcmp(value, "userSpaceOnUse") == 0)
		context->viewport.is_bbox = 1;

	NEMOSHOW_SHADER_AT(one, x0) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "x1"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	NEMOSHOW_SHADER_AT(one, y0) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "y1"), NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	NEMOSHOW_SHADER_AT(one, x1) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "x2"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	NEMOSHOW_SHADER_AT(one, y1) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "y2"), NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");

	pone = context->one;
	context->one = one;

	nemolist_for_each(child, &node->children, link) {
		nemoshow_svg_load_one(context, child);
	}

	context->one = pone;

	context->viewport.is_bbox = 0;

	return 0;
}

static inline int nemoshow_svg_load_radial_gradient(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	struct showone *pone;
	struct xmlnode *child;
	const char *value;
	const char *href;

	one = nemoshow_shader_create(NEMOSHOW_RADIAL_GRADIENT_SHADER);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	href = nemoxml_node_get_attr(node, "xlink:href");
	if (href != NULL)
		NEMOSHOW_SHADER_AT(one, ref) = nemoshow_search_one(context->show, href + 1);

	value = nemoxml_node_get_attr(node, "gradientUnits");
	if (value != NULL && strcmp(value, "userSpaceOnUse") == 0)
		context->viewport.is_bbox = 1;

	NEMOSHOW_SHADER_AT(one, x0) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "fx"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");
	NEMOSHOW_SHADER_AT(one, y0) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "fy"), NEMOSHOW_SVG_ORIENTATION_VERTICAL, "0");
	NEMOSHOW_SHADER_AT(one, r) = nemoshow_svg_get_length(context, nemoxml_node_get_attr(node, "r"), NEMOSHOW_SVG_ORIENTATION_HORIZONTAL, "0");

	pone = context->one;
	context->one = one;

	nemolist_for_each(child, &node->children, link) {
		nemoshow_svg_load_one(context, child);
	}

	context->one = pone;

	context->viewport.is_bbox = 0;

	return 0;
}

static inline int nemoshow_svg_load_stop(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	const char *value;

	one = nemoshow_stop_create();
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	nemoshow_svg_load_style(context, node, one);

	return 0;
}

static inline int nemoshow_svg_load_group(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	struct showone *pone;
	struct xmlnode *child;
	const char *value;

	one = nemoshow_item_create(NEMOSHOW_GROUP_ITEM);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	nemoshow_svg_load_style(context, node, one);

	pone = context->one;
	context->one = one;

	nemolist_for_each(child, &node->children, link) {
		nemoshow_svg_load_one(context, child);
	}

	context->one = pone;

	return 0;
}

static inline int nemoshow_svg_load_defs(struct svgcontext *context, struct xmlnode *node)
{
	struct showone *one;
	struct showone *pone;
	struct xmlnode *child;
	const char *value;

	one = nemoshow_one_create(NEMOSHOW_DEFS_TYPE);
	nemoshow_attach_one(context->show, context->one, one);

	strncpy(one->id, (value = nemoxml_node_get_attr(node, "id")) ? value : "", NEMOSHOW_ID_MAX);

	pone = context->one;
	context->one = one;

	nemolist_for_each(child, &node->children, link) {
		nemoshow_svg_load_one(context, child);
	}

	context->one = pone;

	return 0;
}

static inline int nemoshow_svg_load_one(struct svgcontext *context, struct xmlnode *node)
{
	if (strcmp(node->name, "rect") == 0) {
		nemoshow_svg_load_rect(context, node);
	} else if (strcmp(node->name, "circle") == 0) {
		nemoshow_svg_load_circle(context, node);
	} else if (strcmp(node->name, "path") == 0) {
		nemoshow_svg_load_path(context, node);
	} else if (strcmp(node->name, "linearGradient") == 0) {
		nemoshow_svg_load_linear_gradient(context, node);
	} else if (strcmp(node->name, "radialGradient") == 0) {
		nemoshow_svg_load_radial_gradient(context, node);
	} else if (strcmp(node->name, "stop") == 0) {
		nemoshow_svg_load_stop(context, node);
	} else if (strcmp(node->name, "g") == 0) {
		nemoshow_svg_load_group(context, node);
	} else if (strcmp(node->name, "defs") == 0) {
		nemoshow_svg_load_defs(context, node);
	}

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
		nemoshow_svg_load_one(context, child);
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

int nemoshow_svg_load_uri_path(struct nemoshow *show, struct showone *one, const char *uri)
{
	struct nemoxml *xml;
	struct xmlnode *node;
	struct showone *child;

	if (uri == NULL)
		return -1;

	xml = nemoxml_create();
	nemoxml_load_file(xml, uri);
	nemoxml_update(xml);

	nemolist_for_each(node, &xml->nodes, nodelink) {
		if (strcmp(node->name, "path") == 0) {
			const char *d;

			d = nemoxml_node_get_attr(node, "d");
			if (d != NULL) {
				child = nemoshow_path_create(NEMOSHOW_CMD_PATH);
				nemoobject_sets(&child->object, "d", d, strlen(d));
				nemoshow_attach_one(show, one, child);
			}
		} else if (strcmp(node->name, "rect") == 0) {
			double x = strtod(nemoxml_node_get_attr(node, "x"), NULL);
			double y = strtod(nemoxml_node_get_attr(node, "y"), NULL);
			double width = strtod(nemoxml_node_get_attr(node, "width"), NULL);
			double height = strtod(nemoxml_node_get_attr(node, "height"), NULL);

			child = nemoshow_path_create(NEMOSHOW_RECT_PATH);
			NEMOSHOW_PATH_AT(child, x0) = x;
			NEMOSHOW_PATH_AT(child, y0) = y;
			NEMOSHOW_PATH_AT(child, x1) = x + width;
			NEMOSHOW_PATH_AT(child, y1) = y + height;
			nemoshow_attach_one(show, one, child);
		} else if (strcmp(node->name, "circle") == 0) {
			double x = strtod(nemoxml_node_get_attr(node, "cx"), NULL);
			double y = strtod(nemoxml_node_get_attr(node, "cy"), NULL);
			double r = strtod(nemoxml_node_get_attr(node, "r"), NULL);

			child = nemoshow_path_create(NEMOSHOW_CIRCLE_PATH);
			NEMOSHOW_PATH_AT(child, x0) = x;
			NEMOSHOW_PATH_AT(child, y0) = y;
			NEMOSHOW_PATH_AT(child, r) = r;
			nemoshow_attach_one(show, one, child);
		}
	}

	nemoxml_destroy(xml);

	return 0;
}
