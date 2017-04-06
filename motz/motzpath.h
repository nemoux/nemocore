#ifndef	__NEMOMOTZ_PATH_H__
#define	__NEMOMOTZ_PATH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_PATH_FILL_FLAG = (1 << 0),
	NEMOMOTZ_PATH_STROKE_FLAG = (1 << 1),
	NEMOMOTZ_PATH_TRANSFORM_FLAG = (1 << 2),
	NEMOMOTZ_PATH_RANGE_FLAG = (1 << 3)
} NemoMotzPathFlag;

typedef enum {
	NEMOMOTZ_PATH_SHAPE_DIRTY = (1 << 8),
	NEMOMOTZ_PATH_TRANSFORM_DIRTY = (1 << 9),
	NEMOMOTZ_PATH_COLOR_DIRTY = (1 << 10),
	NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY = (1 << 11),
	NEMOMOTZ_PATH_FONT_DIRTY = (1 << 12),
	NEMOMOTZ_PATH_FONT_SIZE_DIRTY = (1 << 13),
	NEMOMOTZ_PATH_RANGE_DIRTY = (1 << 14)
} NemoMotzPathDirty;

struct motzpath {
	struct motzone one;

	struct tozzstyle *style;
	struct tozzmatrix *matrix;
	struct tozzmatrix *inverse;

	struct tozzpath *path;

	struct tozzpath *subpath;
	float from, to;

	float tx, ty;
	float sx, sy;
	float rz;

	float x, y;
	float w, h;

	float r, g, b, a;

	float stroke_width;

	char *font_path;
	int font_index;
	float font_size;
};

#define NEMOMOTZ_PATH(one)		((struct motzpath *)container_of(one, struct motzpath, one))

extern struct motzone *nemomotz_path_create(void);

extern void nemomotz_path_clear(struct motzone *one);
extern void nemomotz_path_moveto(struct motzone *one, float x, float y);
extern void nemomotz_path_lineto(struct motzone *one, float x, float y);
extern void nemomotz_path_cubicto(struct motzone *one, float x0, float y0, float x1, float y1, float x2, float y2);
extern void nemomotz_path_close(struct motzone *one);
extern void nemomotz_path_text(struct motzone *one, float x, float y, const char *text);
extern void nemomotz_path_cmd(struct motzone *one, const char *cmd);
extern void nemomotz_path_svg(struct motzone *one, const char *url, float x, float y, float w, float h);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, from, from, NEMOMOTZ_PATH_RANGE_DIRTY, NEMOMOTZ_PATH_RANGE_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, from, from);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, to, to, NEMOMOTZ_PATH_RANGE_DIRTY, NEMOMOTZ_PATH_RANGE_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, to, to);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, tx, tx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, tx, tx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, ty, ty, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, ty, ty);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, sx, sx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, sx, sx);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, sy, sy, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, sy, sy);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(path, float, rz, rz, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, rz, rz);

NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, x, x);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, y, y);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, w, width);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, h, height);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, float, r, red, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, r, red);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, float, g, green, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, g, green);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, float, b, blue, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, b, blue);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, float, a, alpha, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, a, alpha);

NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, float, stroke_width, stroke_width, NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, stroke_width, stroke_width);

NEMOMOTZ_DECLARE_DUP_STRING(path, font_path, font_path, NEMOMOTZ_PATH_FONT_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, char *, font_path, font_path);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, int, font_index, font_index, NEMOMOTZ_PATH_FONT_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, int, font_index, font_index);
NEMOMOTZ_DECLARE_SET_ATTRIBUTE(path, float, font_size, font_size, NEMOMOTZ_PATH_FONT_SIZE_DIRTY);
NEMOMOTZ_DECLARE_GET_ATTRIBUTE(path, float, font_size, font_size);

NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, from, from, NEMOMOTZ_PATH_RANGE_DIRTY, NEMOMOTZ_PATH_RANGE_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, to, to, NEMOMOTZ_PATH_RANGE_DIRTY, NEMOMOTZ_PATH_RANGE_FLAG);

NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, tx, tx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, ty, ty, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, sx, sx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, sy, sy, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_DECLARE_SET_TRANSITION_WITH_FLAGS(path, rz, rz, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);

NEMOMOTZ_DECLARE_SET_TRANSITION(path, r, red, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(path, g, green, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(path, b, blue, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_DECLARE_SET_TRANSITION(path, a, alpha, NEMOMOTZ_PATH_COLOR_DIRTY);

NEMOMOTZ_DECLARE_SET_TRANSITION(path, stroke_width, stroke_width, NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY);

NEMOMOTZ_DECLARE_SET_TRANSITION(path, font_size, font_size, NEMOMOTZ_PATH_FONT_SIZE_DIRTY);

NEMOMOTZ_DECLARE_CHECK_TRANSITION_DESTROY(path);
NEMOMOTZ_DECLARE_CHECK_TRANSITION_REVOKE(path);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
