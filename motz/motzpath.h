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
	NEMOMOTZ_PATH_TRANSFORM_FLAG = (1 << 2)
} NemoMotzPathFlag;

typedef enum {
	NEMOMOTZ_PATH_SHAPE_DIRTY = (1 << 8),
	NEMOMOTZ_PATH_TRANSFORM_DIRTY = (1 << 9),
	NEMOMOTZ_PATH_COLOR_DIRTY = (1 << 10),
	NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY = (1 << 11),
	NEMOMOTZ_PATH_FONT_DIRTY = (1 << 12),
	NEMOMOTZ_PATH_FONT_SIZE_DIRTY = (1 << 13),
	NEMOMOTZ_PATH_TEXT_DIRTY = (1 << 14)
} NemoMotzPathDirty;

struct motzpath {
	struct motzone one;

	struct toyzstyle *style;
	struct toyzmatrix *matrix;
	struct toyzmatrix *inverse;

	struct toyzpath *path;

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

#define NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(type, attr, name, dirty)	\
	static inline void nemomotz_path_set_##name(struct motzone *one, type attr) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		path->attr = attr;	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(type, attr, name, dirty, flags)	\
	static inline void nemomotz_path_set_##name(struct motzone *one, type attr) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		path->attr = attr;	\
		nemomotz_one_set_dirty(one, dirty);	\
		nemomotz_one_set_flags(one, flags);	\
	}
#define NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(type, attr, name)	\
	static inline type nemomotz_path_get_##name(struct motzone *one) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		return path->attr;	\
	}
#define NEMOMOTZ_PATH_DECLARE_DUP_STRING(attr, name, dirty)	\
	static inline void nemomotz_path_set_##name(struct motzone *one, const char *attr) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		if (path->attr != NULL) free(path->attr);	\
		path->attr = strdup(attr);	\
		nemomotz_one_set_dirty(one, dirty);	\
	}
#define NEMOMOTZ_PATH_DECLARE_COPY_STRING(attr, name, dirty)	\
	static inline void nemomotz_path_set_##name(struct motzone *one, const char *attr) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		strcpy(path->attr, attr);	\
		nemomotz_one_set_dirty(one, dirty);	\
	}

NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, tx, tx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, tx, tx);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, ty, ty, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, ty, ty);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, sx, sx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, sx, sx);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, sy, sy, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, sy, sy);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE_WITH_FLAGS(float, rz, rz, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, rz, rz);

NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, x, x);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, y, y);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, w, width);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, h, height);

NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(float, r, red, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, r, red);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(float, g, green, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, g, green);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(float, b, blue, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, b, blue);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(float, a, alpha, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, a, alpha);

NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(float, stroke_width, stroke_width, NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, stroke_width, stroke_width);

NEMOMOTZ_PATH_DECLARE_DUP_STRING(font_path, font_path, NEMOMOTZ_PATH_FONT_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(char *, font_path, font_path);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(int, font_index, font_index, NEMOMOTZ_PATH_FONT_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(int, font_index, font_index);
NEMOMOTZ_PATH_DECLARE_SET_ATTRIBUTE(float, font_size, font_size, NEMOMOTZ_PATH_FONT_SIZE_DIRTY);
NEMOMOTZ_PATH_DECLARE_GET_ATTRIBUTE(float, font_size, font_size);

#define NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(type, attr, name, _dirty)	\
	static inline void nemomotz_transition_path_set_##name(struct motztrans *trans, int index, struct motzone *one) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		nemomotz_transition_set_attr(trans, index, &path->attr, path->attr, &one->dirty, _dirty);	\
	}
#define NEMOMOTZ_PATH_DECLARE_SET_TRANSITION_WITH_FLAGS(type, attr, name, _dirty, _flags)	\
	static inline void nemomotz_transition_path_set_##name(struct motztrans *trans, int index, struct motzone *one) {	\
		struct motzpath *path = NEMOMOTZ_PATH(one);	\
		nemomotz_one_set_flags(one, _flags);	\
		nemomotz_transition_set_attr(trans, index, &path->attr, path->attr, &one->dirty, _dirty);	\
	}

NEMOMOTZ_PATH_DECLARE_SET_TRANSITION_WITH_FLAGS(float, tx, tx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION_WITH_FLAGS(float, ty, ty, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION_WITH_FLAGS(float, sx, sx, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION_WITH_FLAGS(float, sy, sy, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION_WITH_FLAGS(float, rz, rz, NEMOMOTZ_PATH_TRANSFORM_DIRTY, NEMOMOTZ_PATH_TRANSFORM_FLAG);

NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(float, r, red, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(float, g, green, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(float, b, blue, NEMOMOTZ_PATH_COLOR_DIRTY);
NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(float, a, alpha, NEMOMOTZ_PATH_COLOR_DIRTY);

NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(float, stroke_width, stroke_width, NEMOMOTZ_PATH_STROKE_WIDTH_DIRTY);

NEMOMOTZ_PATH_DECLARE_SET_TRANSITION(float, font_size, font_size, NEMOMOTZ_PATH_FONT_SIZE_DIRTY);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
