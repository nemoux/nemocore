#ifndef	__NEMOMOTZ_OBJECT_H__
#define	__NEMOMOTZ_OBJECT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

typedef enum {
	NEMOMOTZ_OBJECT_FILL_FLAG = (1 << 0),
	NEMOMOTZ_OBJECT_STROKE_FLAG = (1 << 1)
} NemoMotzObjectFlag;

typedef enum {
	NEMOMOTZ_OBJECT_TRANSFORM_DIRTY = (1 << 8),
	NEMOMOTZ_OBJECT_COLOR_DIRTY = (1 << 9),
	NEMOMOTZ_OBJECT_THICKNESS_DIRTY = (1 << 10)
} NemoMotzObjectDirty;

struct motzobject {
	struct motzone one;

	struct toyzstyle *style;

	float tx, ty;
	float sx, sy;
	float rz;

	float x, y;
	float w, h;
	float radius;

	float r, g, b, a;
	float thickness;
};

#define NEMOMOTZ_OBJECT(one)		((struct motzobject *)container_of(one, struct motzobject, one))

extern struct motzone *nemomotz_object_create(void);

#define NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(type, attr, name)	\
	static inline void nemomotz_object_set_##name(struct motzone *one, type attr) {	\
		struct motzobject *object = (struct motzobject *)container_of(one, struct motzobject, one);	\
		object->attr = attr;	\
	}
#define NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(type, attr, name)	\
	static inline type nemomotz_object_get_##name(struct motzone *one, type attr) {	\
		struct motzobject *object = (struct motzobject *)container_of(one, struct motzobject, one);	\
		return object->attr;	\
	}
#define NEMOMOTZ_OBJECT_DECLARE_DUP_STRING(attr, name)	\
	static inline void nemomotz_object_set_##name(struct motzone *one, const char *attr) {	\
		struct motzobject *object = (struct motzobject *)container_of(one, struct motzobject, one);	\
		if (object->attr != NULL) free(object->attr);	\
		object->attr = strdup(attr);	\
	}
#define NEMOMOTZ_OBJECT_DECLARE_COPY_STRING(attr, name)	\
	static inline void nemomotz_object_set_##name(struct motzone *one, const char *attr) {	\
		struct motzobject *object = (struct motzobject *)container_of(one, struct motzobject, one);	\
		strcpy(object->attr, attr);	\
	}

NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, tx, tx);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, tx, tx);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, ty, ty);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, ty, ty);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, sx, sx);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, sx, sx);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, sy, sy);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, sy, sy);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, rz, rz);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, rz, rz);

NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, x, x);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, x, x);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, y, y);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, y, y);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, w, width);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, w, width);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, h, height);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, h, height);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, radius, radius);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, radius, radius);

NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, r, red);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, r, red);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, g, green);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, g, green);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, b, blue);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, b, blue);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, a, alpha);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, a, alpha);
NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, thickness, thickness);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, thickness, thickness);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
