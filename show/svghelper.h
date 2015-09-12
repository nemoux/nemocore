#ifndef	__NEMOSHOW_SVG_HELPER_H__
#define	__NEMOSHOW_SVG_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define	NEMOSHOW_SVG_DEFAULT_DPI				(100)

typedef enum {
	NEMOSHOW_SVG_PAINT_NONE = 0,
	NEMOSHOW_SVG_PAINT_COLOR = 1,
	NEMOSHOW_SVG_PAINT_GRADIENT = 2,
	NEMOSHOW_SVG_PAINT_PATTERN = 3
} NemoShowSvgPaintType;

typedef enum {
	NEMOSHOW_SVG_GRADIENT_LINEAR = 0,
	NEMOSHOW_SVG_GRADIENT_RADIAL = 1
} NemoShowSvgGradientType;

typedef enum {
	NEMOSHOW_SVG_ASPECT_RATIO_UNKNOWN = 0,
	NEMOSHOW_SVG_ASPECT_RATIO_NONE = 1,
	NEMOSHOW_SVG_ASPECT_RATIO_XMINYMIN = 2,
	NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMIN = 3,
	NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMIN = 4,
	NEMOSHOW_SVG_ASPECT_RATIO_XMINYMID = 5,
	NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMID = 6,
	NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMID = 7,
	NEMOSHOW_SVG_ASPECT_RATIO_XMINYMAX = 8,
	NEMOSHOW_SVG_ASPECT_RATIO_XMIDYMAX = 9,
	NEMOSHOW_SVG_ASPECT_RATIO_XMAXYMAX = 10
} NemoShowSvgAspectRatio;

typedef enum {
	NEMOSHOW_SVG_MEETSLICE_UNKNOWN = 0,
	NEMOSHOW_SVG_MEETSLICE_MEET = 1,
	NEMOSHOW_SVG_MEETSLICE_SLICE = 2
} NemoShowSvgMeetSlice;

typedef enum {
	NEMOSHOW_SVG_ORIENTATION_OTHER = 0,
	NEMOSHOW_SVG_ORIENTATION_HORIZONTAL = 1,
	NEMOSHOW_SVG_ORIENTATION_VERTICAL = 2
} NemoShowSvgOrientation;

struct svgcontext {
	struct nemoshow *show;
	struct showitem *svg;
	struct showone *one;

	struct nemoxml *xml;

	struct {
		uint32_t width, height;

		int is_bbox;
	} viewport;

	uint32_t width, height;

	double fontsize;
	double rgba[4];
};

extern int nemoshow_svg_load_uri(struct nemoshow *show, struct showone *one, const char *uri);
extern int nemoshow_svg_load_uri_path(struct nemoshow *show, struct showone *one, const char *uri);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
