#ifndef	__NEMO_ATTR_H__
#define	__NEMO_ATTR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nemomisc.h>

#define	NEMOATTR_NAME_MAX		(32)

struct nemoattr {
	char name[NEMOATTR_NAME_MAX];
	uint32_t serial;

	void *p;
	int size;

	union {
		uint32_t i;
		uint64_t l;
		float f;
		double d;
	} v;

	int needs_free;
};

struct nemoobject {
	struct nemoattr *attrs;
	int nattrs;
	int mattrs;
};

static inline const char *nemoattr_get_name(struct nemoattr *attr)
{
	return attr->name;
}

static inline void nemoattr_set_serial(struct nemoattr *attr, uint32_t serial)
{
	attr->serial = serial;
}

static inline uint32_t nemoattr_get_serial(struct nemoattr *attr)
{
	return attr->serial;
}

static inline void nemoattr_seti(struct nemoattr *attr, uint32_t i)
{
	*(uint32_t *)(attr->p) = i;
}

static inline uint32_t nemoattr_geti(struct nemoattr *attr)
{
	return *(uint32_t *)(attr->p);
}

static inline void nemoattr_seti_offset(struct nemoattr *attr, int offset, uint32_t i)
{
	((uint32_t *)(attr->p))[offset] = i;
}

static inline uint32_t nemoattr_geti_offset(struct nemoattr *attr, int offset)
{
	return ((uint32_t *)(attr->p))[offset];
}

static inline void nemoattr_setl(struct nemoattr *attr, uint64_t l)
{
	*(uint64_t *)(attr->p) = l;
}

static inline uint64_t nemoattr_getl(struct nemoattr *attr)
{
	return *(uint64_t *)(attr->p);
}

static inline void nemoattr_setl_offset(struct nemoattr *attr, int offset, uint64_t l)
{
	((uint64_t *)(attr->p))[offset] = l;
}

static inline uint64_t nemoattr_getl_offset(struct nemoattr *attr, int offset)
{
	return ((uint64_t *)(attr->p))[offset];
}

static inline void nemoattr_setf(struct nemoattr *attr, float f)
{
	*(float *)(attr->p) = f;
}

static inline float nemoattr_getf(struct nemoattr *attr)
{
	return *(float *)(attr->p);
}

static inline void nemoattr_setf_offset(struct nemoattr *attr, int offset, float f)
{
	((float *)(attr->p))[offset] = f;
}

static inline float nemoattr_getf_offset(struct nemoattr *attr, int offset)
{
	return ((float *)(attr->p))[offset];
}

static inline void nemoattr_setd(struct nemoattr *attr, double d)
{
	*(double *)(attr->p) = d;
}

static inline double nemoattr_getd(struct nemoattr *attr)
{
	return *(double *)(attr->p);
}

static inline void nemoattr_setd_offset(struct nemoattr *attr, int offset, double d)
{
	((double *)(attr->p))[offset] = d;
}

static inline double nemoattr_getd_offset(struct nemoattr *attr, int offset)
{
	return ((double *)(attr->p))[offset];
}

static inline void nemoattr_sets(struct nemoattr *attr, const char *s, int size)
{
	if (attr->needs_free != 0 && attr->size < size + 1) {
		free(attr->p);

		attr->p = malloc(size + 1);
		attr->size = size + 1;
		attr->needs_free = 1;
	} else if (attr->p == NULL) {
		attr->p = malloc(size + 1);
		attr->size = size + 1;
		attr->needs_free = 1;
	}

	strncpy((char *)(attr->p), s, size);
	((char *)attr->p)[size] = '\0';
}

static inline const char *nemoattr_gets(struct nemoattr *attr)
{
	return (const char *)(attr->p);
}

static inline void nemoattr_setp(struct nemoattr *attr, void *p)
{
	if (p == NULL)
		attr->p = &attr->v.d;
	else
		attr->p = p;
}

static inline void *nemoattr_getp(struct nemoattr *attr)
{
	return attr->p;
}

static inline void nemoattr_put(struct nemoattr *attr)
{
	attr->name[0] = '\0';

	if (attr->needs_free != 0) {
		free(attr->p);

		attr->needs_free = 0;
	}
}

static inline struct nemoattr *nemoobject_set(struct nemoobject *object, const char *name)
{
	int i = object->nattrs;

	if (object->nattrs >= object->mattrs)
		return NULL;

	strncpy(object->attrs[i].name, name, NEMOATTR_NAME_MAX - 1);
	object->attrs[i].name[MIN(strlen(name), NEMOATTR_NAME_MAX - 1)] = '\0';

	object->nattrs++;

	return &object->attrs[i];
}

static inline struct nemoattr *nemoobject_get(struct nemoobject *object, const char *name)
{
	int i;

	for (i = 0; i < object->nattrs; i++) {
		if (strcmp(object->attrs[i].name, name) == 0) {
			return &object->attrs[i];
		}
	}

	return NULL;
}

static inline struct nemoattr *nemoobject_set_reserved(struct nemoobject *object, const char *name, void *p, int size)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr == NULL)
		attr = nemoobject_set(object, name);
	if (attr != NULL) {
		if (attr->needs_free != 0)
			free(attr->p);

		attr->p = p;
		attr->size = size;
		attr->needs_free = 0;
	}

	return attr;
}

static inline int nemoobject_has(struct nemoobject *object, const char *name)
{
	int i;

	for (i = 0; i < object->nattrs; i++) {
		if (strcmp(object->attrs[i].name, name) == 0) {
			return i;
		}
	}

	return -1;
}

static inline void nemoobject_seti(struct nemoobject *object, const char *name, uint32_t i)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		*(uint32_t *)(attr->p) = i;
	} else {
		attr = nemoobject_set(object, name);

		attr->v.i = i;

		attr->p = &attr->v.i;
		attr->needs_free = 0;
	}
}

static inline uint32_t nemoobject_geti(struct nemoobject *object, const char *name)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return *(uint32_t *)(attr->p);
	}

	return 0;
}

static inline void nemoobject_seti_offset(struct nemoobject *object, const char *name, int offset, uint32_t i)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		((uint32_t *)(attr->p))[offset] = i;
	}
}

static inline uint32_t nemoobject_geti_offset(struct nemoobject *object, const char *name, int offset)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return ((uint32_t *)(attr->p))[offset];
	}

	return 0;
}

static inline void nemoobject_setl(struct nemoobject *object, const char *name, uint64_t l)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		*(uint64_t *)(attr->p) = l;
	} else {
		attr = nemoobject_set(object, name);

		attr->v.l = l;

		attr->p = &attr->v.l;
		attr->needs_free = 0;
	}
}

static inline uint64_t nemoobject_getl(struct nemoobject *object, const char *name)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return *(uint64_t *)(attr->p);
	}

	return 0;
}

static inline void nemoobject_setl_offset(struct nemoobject *object, const char *name, int offset, uint64_t l)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		((uint64_t *)(attr->p))[offset] = l;
	}
}

static inline uint64_t nemoobject_getl_offset(struct nemoobject *object, const char *name, int offset)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return ((uint64_t *)(attr->p))[offset];
	}

	return 0;
}

static inline void nemoobject_setf(struct nemoobject *object, const char *name, float f)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		*(float *)(attr->p) = f;
	} else {
		attr = nemoobject_set(object, name);

		attr->v.f = f;

		attr->p = &attr->v.f;
		attr->needs_free = 0;
	}
}

static inline float nemoobject_getf(struct nemoobject *object, const char *name)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return *(float *)(attr->p);
	}

	return 0.0f;
}

static inline void nemoobject_setf_offset(struct nemoobject *object, const char *name, int offset, float f)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		((float *)(attr->p))[offset] = f;
	}
}

static inline float nemoobject_getf_offset(struct nemoobject *object, const char *name, int offset)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return ((float *)(attr->p))[offset];
	}

	return 0.0f;
}

static inline void nemoobject_setd(struct nemoobject *object, const char *name, double d)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		*(double *)(attr->p) = d;
	} else {
		attr = nemoobject_set(object, name);

		attr->v.d = d;

		attr->p = &attr->v.d;
		attr->needs_free = 0;
	}
}

static inline double nemoobject_getd(struct nemoobject *object, const char *name)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return *(double *)(attr->p);
	}

	return 0.0f;
}

static inline void nemoobject_setd_offset(struct nemoobject *object, const char *name, int offset, double d)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		((double *)(attr->p))[offset] = d;
	}
}

static inline double nemoobject_getd_offset(struct nemoobject *object, const char *name, int offset)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return ((double *)(attr->p))[offset];
	}

	return 0.0f;
}

static inline void nemoobject_sets(struct nemoobject *object, const char *name, const char *s, int size)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		if (attr->needs_free != 0 && attr->size < size + 1) {
			free(attr->p);

			attr->p = malloc(size + 1);
			attr->size = size + 1;
			attr->needs_free = 1;
		}

		strncpy((char *)(attr->p), s, size);
		((char *)attr->p)[size] = '\0';
	} else {
		attr = nemoobject_set(object, name);

		attr->p = malloc(size + 1);
		attr->size = size + 1;
		attr->needs_free = 1;

		strncpy((char *)(attr->p), s, size);
		((char *)attr->p)[size] = '\0';
	}
}

static inline const char *nemoobject_gets(struct nemoobject *object, const char *name)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return (const char *)(attr->p);
	}

	return NULL;
}

static inline void nemoobject_setp(struct nemoobject *object, const char *name, void *p)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		attr->p = p;
	} else {
		attr = nemoobject_set(object, name);

		attr->p = p;
		attr->needs_free = 0;
	}
}

static inline void *nemoobject_getp(struct nemoobject *object, const char *name)
{
	struct nemoattr *attr;

	attr = nemoobject_get(object, name);
	if (attr != NULL) {
		return attr->p;
	}

	return NULL;
}

static inline struct nemoattr *nemoobject_iget(struct nemoobject *object, int index)
{
	return &object->attrs[index];
}

static inline void nemoobject_iseti(struct nemoobject *object, int index, uint32_t i)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	*(uint32_t *)(attr->p) = i;
}

static inline uint32_t nemoobject_igeti(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return *(uint32_t *)(attr->p);
}

static inline void nemoobject_iseti_offset(struct nemoobject *object, int index, int offset, uint32_t i)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	((uint32_t *)(attr->p))[offset] = i;
}

static inline uint32_t nemoobject_igeti_offset(struct nemoobject *object, int index, int offset)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return ((uint32_t *)(attr->p))[offset];
}

static inline void nemoobject_isetl(struct nemoobject *object, int index, uint64_t l)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	*(uint64_t *)(attr->p) = l;
}

static inline uint64_t nemoobject_igetl(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return *(uint64_t *)(attr->p);
}

static inline void nemoobject_isetl_offset(struct nemoobject *object, int index, int offset, uint64_t l)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	((uint64_t *)(attr->p))[offset] = l;
}

static inline uint64_t nemoobject_igetl_offset(struct nemoobject *object, int index, int offset)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return ((uint64_t *)(attr->p))[offset];
}

static inline void nemoobject_isetf(struct nemoobject *object, int index, float f)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	*(float *)(attr->p) = f;
}

static inline float nemoobject_igetf(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return *(float *)(attr->p);
}

static inline void nemoobject_isetf_offset(struct nemoobject *object, int index, int offset, float f)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	((float *)(attr->p))[offset] = f;
}

static inline float nemoobject_igetf_offset(struct nemoobject *object, int index, int offset)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return ((float *)(attr->p))[offset];
}

static inline void nemoobject_isetd(struct nemoobject *object, int index, double d)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	*(double *)(attr->p) = d;
}

static inline double nemoobject_igetd(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return *(double *)(attr->p);
}

static inline void nemoobject_isetd_offset(struct nemoobject *object, int index, int offset, double d)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	((double *)(attr->p))[offset] = d;
}

static inline double nemoobject_igetd_offset(struct nemoobject *object, int index, int offset)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return ((double *)(attr->p))[offset];
}

static inline void nemoobject_isets(struct nemoobject *object, int index, const char *s, int size)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	if (attr->needs_free != 0 && attr->size < size + 1) {
		free(attr->p);

		attr->p = malloc(size + 1);
		attr->size = size + 1;
		attr->needs_free = 1;
	}

	strncpy((char *)(attr->p), s, size);
	((char *)attr->p)[size] = '\0';
}

static inline const char *nemoobject_igets(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return (const char *)(attr->p);
}

static inline void nemoobject_isetp(struct nemoobject *object, int index, void *p)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	attr->p = p;
}

static inline void *nemoobject_igetp(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return attr->p;
}

static inline void nemoobject_prepare(struct nemoobject *object, int mattrs)
{
	object->attrs = (struct nemoattr *)malloc(sizeof(struct nemoattr) * mattrs);
	if (object->attrs != NULL) {
		memset(object->attrs, 0, sizeof(struct nemoattr) * mattrs);

		object->nattrs = 0;
		object->mattrs = mattrs;
	} else {
		object->nattrs = 0;
		object->mattrs = 0;
	}
}

static inline void nemoobject_finish(struct nemoobject *object)
{
	int i;

	for (i = 0; i < object->nattrs; i++) {
		nemoattr_put(&object->attrs[i]);
	}

	free(object->attrs);
}

static inline struct nemoobject *nemoobject_create(int mattrs)
{
	struct nemoobject *object;

	object = (struct nemoobject *)malloc(sizeof(struct nemoobject));
	if (object == NULL)
		return NULL;

	nemoobject_prepare(object, mattrs);

	return object;
}

static inline void nemoobject_destroy(struct nemoobject *object)
{
	nemoobject_finish(object);

	free(object);
}

static inline int nemoobject_get_count(struct nemoobject *object)
{
	return object->nattrs;
}

static inline const char *nemoobject_get_name(struct nemoobject *object, int index)
{
	struct nemoattr *attr = nemoobject_iget(object, index);

	return attr->name;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
