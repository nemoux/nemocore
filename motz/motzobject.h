#ifndef	__NEMOMOTZ_OBJECT_H__
#define	__NEMOMOTZ_OBJECT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

#include <nemomotz.h>

struct motzobject {
	struct motzone one;

	struct toyzstyle *style;

	float tx, ty;
	float sx, sy;
	float rz;

	float r;

	struct {
		float r, g, b, a;

		int has;
	} fill;

	struct {
		float r, g, b, a;

		float w;

		int has;
	} stroke;
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
#define NEMOMOTZ_OBJECT_DECLARE_SET_COLOR(type, attr, name)	\
	static inline void nemomotz_object_set_##name(struct motzone *one, type r, type g, type b, type a) {	\
		struct motzobject *object = (struct motzobject *)container_of(one, struct motzobject, one);	\
		object->attr.r = r;	\
		object->attr.g = g;	\
		object->attr.b = b;	\
		object->attr.a = a;	\
		object->attr.has = 1;	\
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

NEMOMOTZ_OBJECT_DECLARE_SET_ATTRIBUTE(float, r, r);
NEMOMOTZ_OBJECT_DECLARE_GET_ATTRIBUTE(float, r, r);

NEMOMOTZ_OBJECT_DECLARE_SET_COLOR(float, fill, fill);
NEMOMOTZ_OBJECT_DECLARE_SET_COLOR(float, stroke, stroke);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
