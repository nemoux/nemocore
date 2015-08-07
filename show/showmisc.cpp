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
		{ "alpha",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "begin",						NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY },
		{ "blur",							NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "child",						NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY },
		{ "d",								NEMOSHOW_STRING_PROP,				NEMOSHOW_TEXT_DIRTY },
		{ "ease",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "end",							NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY },
		{ "event",						NEMOSHOW_INTEGER_PROP,			NEMOSHOW_NONE_DIRTY },
		{ "fill",							NEMOSHOW_COLOR_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "flags",						NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "font",							NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "font-size",				NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "from",							NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "height",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "id",								NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "matrix",						NEMOSHOW_STRING_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "name",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "offset",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "r",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "rx",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "ry",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "shader",						NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "src",							NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "stroke",						NEMOSHOW_COLOR_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "stroke-width",			NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY | NEMOSHOW_STYLE_DIRTY },
		{ "style",						NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "t",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "timing",						NEMOSHOW_STRING_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "to",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "type",							NEMOSHOW_STRING_PROP,				NEMOSHOW_STYLE_DIRTY },
		{ "value",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_NONE_DIRTY },
		{ "width",						NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "x",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "x0",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "x1",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "x2",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "y",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "y0",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "y1",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
		{ "y2",								NEMOSHOW_DOUBLE_PROP,				NEMOSHOW_SHAPE_DIRTY },
	};

	return (struct showprop *)bsearch(name,
			props,
			sizeof(props) / sizeof(props[0]),
			sizeof(props[0]),
			nemoshow_compare_property);
}
