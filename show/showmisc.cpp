#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showmisc.h>

static int nemoshow_compare_property(const void *a, const void *b)
{
	return strcasecmp((const char *)a, (const char *)b);
}

struct showprop *nemoshow_get_property(const char *name)
{
	static struct showprop props[] = {
		{ "alpha",						NEMOSHOW_DOUBLE_PROP },
		{ "begin",						NEMOSHOW_INTEGER_PROP },
		{ "d",								NEMOSHOW_STRING_PROP },
		{ "ease",							NEMOSHOW_STRING_PROP },
		{ "end",							NEMOSHOW_INTEGER_PROP },
		{ "event",						NEMOSHOW_INTEGER_PROP },
		{ "fill",							NEMOSHOW_COLOR_PROP },
		{ "font",							NEMOSHOW_STRING_PROP },
		{ "font-size",				NEMOSHOW_DOUBLE_PROP },
		{ "from",							NEMOSHOW_DOUBLE_PROP },
		{ "height",						NEMOSHOW_DOUBLE_PROP },
		{ "id",								NEMOSHOW_STRING_PROP },
		{ "matrix",						NEMOSHOW_STRING_PROP },
		{ "r",								NEMOSHOW_DOUBLE_PROP },
		{ "rx",								NEMOSHOW_DOUBLE_PROP },
		{ "ry",								NEMOSHOW_DOUBLE_PROP },
		{ "src",							NEMOSHOW_STRING_PROP },
		{ "stroke",						NEMOSHOW_COLOR_PROP },
		{ "stroke-width",			NEMOSHOW_DOUBLE_PROP },
		{ "style",						NEMOSHOW_STRING_PROP },
		{ "t",								NEMOSHOW_DOUBLE_PROP },
		{ "timing",						NEMOSHOW_STRING_PROP },
		{ "to",								NEMOSHOW_DOUBLE_PROP },
		{ "type",							NEMOSHOW_STRING_PROP },
		{ "width",						NEMOSHOW_DOUBLE_PROP },
		{ "x",								NEMOSHOW_DOUBLE_PROP },
		{ "x0",								NEMOSHOW_DOUBLE_PROP },
		{ "x1",								NEMOSHOW_DOUBLE_PROP },
		{ "x2",								NEMOSHOW_DOUBLE_PROP },
		{ "y",								NEMOSHOW_DOUBLE_PROP },
		{ "y0",								NEMOSHOW_DOUBLE_PROP },
		{ "y1",								NEMOSHOW_DOUBLE_PROP },
		{ "y2",								NEMOSHOW_DOUBLE_PROP },
	};

	return (struct showprop *)bsearch(name,
			props,
			sizeof(props) / sizeof(props[0]),
			sizeof(props[0]),
			nemoshow_compare_property);
}
