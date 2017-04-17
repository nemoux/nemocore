#ifndef	__NEMO_ITEM_H__
#define	__NEMO_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>

struct itemattr {
	char *name;
	char *value;

	uint32_t tag;

	struct nemolist link;
};

struct itemone {
	char *path;

	struct nemolist list;
	int count;

	struct nemolist link;
};

struct itembox {
	struct itemone **ones;
	int nones;
};

struct itemarray {
	struct itemattr **attrs;
	int nattrs;
};

struct nemoitem {
	struct nemolist list;
	int count;
};

#define nemoitem_for_each(one, item)	\
	nemolist_for_each(one, &((item)->list), link)
#define nemoitem_for_each_safe(one, tmp, item)	\
	nemolist_for_each_safe(one, tmp, &((item)->list), link)

#define nemoitem_attr_for_each(attr, one)	\
	nemolist_for_each(attr, &((one)->list), link)
#define nemoitem_attr_for_each_safe(attr, tmp, one)	\
	nemolist_for_each_safe(attr, tmp, &((one)->list), link)

extern struct nemoitem *nemoitem_create(void);
extern void nemoitem_destroy(struct nemoitem *item);

extern void nemoitem_attach_one(struct nemoitem *item, struct itemone *one);
extern void nemoitem_detach_one(struct nemoitem *item, struct itemone *one);

extern struct itemone *nemoitem_search_one(struct nemoitem *item, const char *path);
extern struct itemone *nemoitem_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value);
extern struct itemone *nemoitem_search_attr2(struct nemoitem *item, const char *path, const char *name0, const char *value0, const char *name1, const char *value1);
extern struct itemone *nemoitem_search_attr3(struct nemoitem *item, const char *path, const char *name0, const char *value0, const char *name1, const char *value1, const char *name2, const char *value2);
extern struct itemone *nemoitem_search_attrs(struct nemoitem *item, const char *path, char delimiter, const char *attrs);
extern struct itemone *nemoitem_search_format(struct nemoitem *item, const char *path, char delimiter, const char *fmt, ...);
extern int nemoitem_count_one(struct nemoitem *item, const char *path);

extern struct itembox *nemoitem_box_search_one(struct nemoitem *item, const char *path);
extern struct itembox *nemoitem_box_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value);
extern struct itembox *nemoitem_box_search_attrs(struct nemoitem *item, const char *path, char delimiter, const char *attrs);
extern struct itembox *nemoitem_box_search_format(struct nemoitem *item, const char *path, char delimiter, const char *fmt, ...);
extern void nemoitem_box_destroy(struct itembox *box);

extern struct itemarray *nemoitem_array_search_attr(struct itemone *one, const char *name);
extern void nemoitem_array_destroy(struct itemarray *array);

extern struct itemone *nemoitem_one_create(void);
extern void nemoitem_one_destroy(struct itemone *one);

extern struct itemone *nemoitem_one_clone(struct itemone *one);
extern void nemoitem_one_copy(struct itemone *done, struct itemone *sone);

extern void nemoitem_one_set_path(struct itemone *one, const char *path);
extern void nemoitem_one_set_path_format(struct itemone *one, const char *fmt, ...);
extern int nemoitem_one_has_path(struct itemone *one, const char *path);
extern int nemoitem_one_has_path_prefix(struct itemone *one, const char *prefix);
extern int nemoitem_one_has_path_suffix(struct itemone *one, const char *suffix);
extern int nemoitem_one_has_path_format(struct itemone *one, const char *fmt, ...);
extern int nemoitem_one_has_path_regex(struct itemone *one, const char *expr);

extern int nemoitem_one_set_attr(struct itemone *one, const char *name, const char *value);
extern int nemoitem_one_set_attr_format(struct itemone *one, const char *name, const char *fmt, ...);
extern const char *nemoitem_one_get_attr(struct itemone *one, const char *name);
extern void nemoitem_one_put_attr(struct itemone *one, const char *name);
extern int nemoitem_one_has_attr(struct itemone *one, const char *name);

extern int nemoitem_one_set_attr_tag(struct itemone *one, const char *name, uint32_t tag, const char *value);
extern int nemoitem_one_set_attr_tag_format(struct itemone *one, const char *name, uint32_t tag, const char *fmt, ...);
extern const char *nemoitem_one_get_attr_tag(struct itemone *one, const char *name, uint32_t tag);
extern void nemoitem_one_put_attr_tag(struct itemone *one, const char *name, uint32_t tag);
extern int nemoitem_one_has_attr_tag(struct itemone *one, const char *name, uint32_t tag);

extern int nemoitem_one_load_attrs(struct itemone *one, const char *buffer, char delimiter);
extern int nemoitem_one_save_attrs(struct itemone *one, char *buffer, char delimiter);

extern int nemoitem_one_save_format(struct itemone *one, char *buffer, const char *atpath, const char *atattr, const char *atvalue);

extern int nemoitem_one_load_textline(struct itemone *one, const char *buffer, char delimiter);
extern int nemoitem_one_save_textline(struct itemone *one, char *buffer, char delimiter);

extern int nemoitem_load_textfile(struct nemoitem *item, const char *filepath, char delimiter);
extern int nemoitem_save_textfile(struct nemoitem *item, const char *filepath, char delimiter);

static inline int nemoitem_get_count(struct nemoitem *item)
{
	return item->count;
}

static inline const char *nemoitem_one_get_path(struct itemone *one)
{
	return one->path;
}

static inline int nemoitem_one_get_iattr(struct itemone *one, const char *name, int value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? strtoul(str, NULL, 10) : value;
}

static inline float nemoitem_one_get_fattr(struct itemone *one, const char *name, float value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? strtod(str, NULL) : value;
}

static inline const char *nemoitem_one_get_sattr(struct itemone *one, const char *name, const char *value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? str : value;
}

static inline int nemoitem_one_has_iattr(struct itemone *one, const char *name, int value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? strtol(str, NULL, 10) == value : 0;
}

static inline int nemoitem_one_has_fattr(struct itemone *one, const char *name, float value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? strtod(str, NULL) == value : 0;
}

static inline int nemoitem_one_has_sattr(struct itemone *one, const char *name, const char *value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? strcmp(str, value) == 0 : 0;
}

static inline int nemoitem_box_get_count(struct itembox *box)
{
	return box->nones;
}

static inline struct itemone *nemoitem_box_get_one(struct itembox *box, int index)
{
	return box->ones[index];
}

static inline const char *nemoitem_box_get_path(struct itembox *box, int index)
{
	return nemoitem_one_get_path(box->ones[index]);
}

static inline int nemoitem_box_get_iattr(struct itembox *box, int index, const char *name, int value)
{
	return nemoitem_one_get_iattr(box->ones[index], name, value);
}

static inline float nemoitem_box_get_fattr(struct itembox *box, int index, const char *name, float value)
{
	return nemoitem_one_get_fattr(box->ones[index], name, value);
}

static inline const char *nemoitem_box_get_sattr(struct itembox *box, int index, const char *name, const char *value)
{
	return nemoitem_one_get_sattr(box->ones[index], name, value);
}

static inline int nemoitem_array_get_count(struct itemarray *array)
{
	return array->nattrs;
}

static inline struct itemattr *nemoitem_array_get_attr(struct itemarray *array, int index)
{
	return array->attrs[index];
}

static inline int nemoitem_array_get_iattr(struct itemarray *array, int index, int value)
{
	const char *str = array->attrs[index]->value;

	return str != NULL ? strtoul(str, NULL, 10) : value;
}

static inline float nemoitem_array_get_fattr(struct itemarray *array, int index, float value)
{
	const char *str = array->attrs[index]->value;

	return str != NULL ? strtod(str, NULL) : value;
}

static inline const char *nemoitem_array_get_sattr(struct itemarray *array, int index, const char *value)
{
	const char *str = array->attrs[index]->value;

	return str != NULL ? str : value;
}

static inline void nemoitem_attr_set_name(struct itemattr *attr, const char *name)
{
	free(attr->name);

	attr->name = strdup(name);
}

static inline const char *nemoitem_attr_get_name(struct itemattr *attr)
{
	return attr->name;
}

static inline void nemoitem_attr_set_value(struct itemattr *attr, const char *value)
{
	free(attr->value);

	attr->value = strdup(value);
}

static inline const char *nemoitem_attr_get_value(struct itemattr *attr)
{
	return attr->value;
}

static inline void nemoitem_attr_set_tag(struct itemattr *attr, uint32_t tag)
{
	attr->tag = tag;
}

static inline uint32_t nemoitem_attr_get_tag(struct itemattr *attr)
{
	return attr->tag;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
