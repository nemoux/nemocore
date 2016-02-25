#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showone.h>
#include <showmisc.h>

static int nemoshow_compare_property(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

struct showprop *nemoshow_get_property(const char *name)
{
	static struct showprop props[] = {
		{ "alpha",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "ambient",					NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_FILTER_DIRTY,			0x0 },
		{ "attr",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "ax",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "ay",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "begin",						NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "child",						NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "clip",							NEMOSHOW_STRING_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "color",						NEMOSHOW_FLOAT_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "d",								NEMOSHOW_STRING_PROP,				NEMOSHOW_TEXT_DIRTY,				0x0 },
		{ "diffuse",					NEMOSHOW_FLOAT_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "dx",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_FILTER_DIRTY,			0x0 },
		{ "dy",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_FILTER_DIRTY,			0x0 },
		{ "dz",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_FILTER_DIRTY,			0x0 },
		{ "ease",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "element",					NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "end",							NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "event",						NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "fill",							NEMOSHOW_COLOR_PROP,				NEMOSHOW_STYLE_DIRTY,				NEMOSHOW_FILL_STATE },
		{ "filter",						NEMOSHOW_STRING_PROP,				NEMOSHOW_FILTER_DIRTY,			0x0 },
		{ "flags",						NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "font",							NEMOSHOW_STRING_PROP,				NEMOSHOW_TEXT_DIRTY,				0x0 },
		{ "font-family",			NEMOSHOW_STRING_PROP,				NEMOSHOW_TEXT_DIRTY,				0x0 },
		{ "font-layout",			NEMOSHOW_STRING_PROP,				NEMOSHOW_TEXT_DIRTY,				0x0 },
		{ "font-size",				NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_TEXT_DIRTY,				0x0 },
		{ "font-style",				NEMOSHOW_STRING_PROP,				NEMOSHOW_TEXT_DIRTY,				0x0 },
		{ "from",							NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "height",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "id",								NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "inner",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "light",						NEMOSHOW_FLOAT_PROP,				NEMOSHOW_REDRAW_DIRTY,			0x0 },
		{ "matrix",						NEMOSHOW_FLOAT_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "mode",							NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "name",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "normal",						NEMOSHOW_FLOAT_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "offset",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "path",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "pathdash",					NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_PATH_DIRTY,				0x0 },
		{ "pathdashcount",		NEMOSHOW_INTEGER_PROP,			NEMOSHOW_PATH_DIRTY,				0x0 },
		{ "pathdeviation",		NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_PATH_DIRTY,				0x0 },
		{ "pathseed",					NEMOSHOW_INTEGER_PROP,			NEMOSHOW_PATH_DIRTY,				0x0 },
		{ "pathsegment",			NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_PATH_DIRTY,				0x0 },
		{ "px",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "py",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "r",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "ro",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "rx",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "ry",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "rz",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "shader",						NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "specular",					NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_FILTER_DIRTY,			0x0 },
		{ "src",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "stroke",						NEMOSHOW_COLOR_PROP,				NEMOSHOW_STYLE_DIRTY,				NEMOSHOW_STROKE_STATE },
		{ "stroke-width",			NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY,			0x0 },
		{ "style",						NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "sx",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "sy",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "sz",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "t",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "texcoord",					NEMOSHOW_FLOAT_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "timing",						NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "to",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "tx",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "ty",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "type",							NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY,				0x0 },
		{ "tz",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_MATRIX_DIRTY,			0x0 },
		{ "uri",							NEMOSHOW_STRING_PROP,				NEMOSHOW_URI_DIRTY,					0x0 },
		{ "value",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_NONE_DIRTY,				0x0 },
		{ "vertex",						NEMOSHOW_FLOAT_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "width",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "x",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "x0",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "x1",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "x2",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "y",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "y0",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "y1",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
		{ "y2",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY,				0x0 },
	};

	return (struct showprop *)bsearch(name,
			props,
			sizeof(props) / sizeof(props[0]),
			sizeof(props[0]),
			nemoshow_compare_property);
}
