#ifndef	__NEMO_ITEM_H__
#define	__NEMO_ITEM_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct itemattr {
	char *name;
	char *value;

	struct nemolist link;
};

struct itemone {
	char *path;

	struct nemolist list;

	struct nemolist link;
};

struct itembox {
	struct itemone **ones;
	int nones;
};

struct nemoitem {
	struct nemolist list;
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
extern int nemoitem_count_one(struct nemoitem *item, const char *path);

extern struct itembox *nemoitem_box_search_one(struct nemoitem *item, const char *path);
extern struct itembox *nemoitem_box_search_attr(struct nemoitem *item, const char *path, const char *name, const char *value);
extern void nemoitem_box_destroy(struct itembox *box);

extern struct itemone *nemoitem_one_create(void);
extern void nemoitem_one_destroy(struct itemone *one);

extern struct itemone *nemoitem_one_clone(struct itemone *one);

extern int nemoitem_one_load(struct itemone *one, const char *buffer);
extern int nemoitem_one_save(struct itemone *one, char *buffer);

extern void nemoitem_one_set_path(struct itemone *one, const char *path);
extern const char *nemoitem_one_get_path(struct itemone *one);
extern int nemoitem_one_has_path(struct itemone *one, const char *path);

extern int nemoitem_one_set_attr(struct itemone *one, const char *name, const char *value);
extern const char *nemoitem_one_get_attr(struct itemone *one, const char *name);
extern void nemoitem_one_put_attr(struct itemone *one, const char *name);
extern int nemoitem_one_has_attr(struct itemone *one, const char *name, const char *value);

extern int nemoitem_load(struct nemoitem *item, const char *filepath);
extern int nemoitem_save(struct nemoitem *item, const char *filepath);

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

	return str != NULL ? strtoul(str, NULL, 10) == value : 0;
}

static inline int nemoitem_one_has_fattr(struct itemone *one, const char *name, float value)
{
	const char *str = nemoitem_one_get_attr(one, name);

	return str != NULL ? strtod(str, NULL) == value : 0;
}

static inline int nemoitem_one_has_sattr(struct itemone *one, const char *name, const char *value)
{
	return nemoitem_one_has_attr(one, name, value);
}

static inline int nemoitem_get_iattr(struct nemoitem *item, const char *path, const char *name, int value)
{
	struct itemone *one = nemoitem_search_one(item, path);
	const char *str = one != NULL ? nemoitem_one_get_attr(one, name) : NULL;

	return str != NULL ? strtoul(str, NULL, 10) : value;
}

static inline float nemoitem_get_fattr(struct nemoitem *item, const char *path, const char *name, float value)
{
	struct itemone *one = nemoitem_search_one(item, path);
	const char *str = one != NULL ? nemoitem_one_get_attr(one, name) : NULL;

	return str != NULL ? strtod(str, NULL) : value;
}

static inline const char *nemoitem_get_sattr(struct nemoitem *item, const char *path, const char *name, const char *value)
{
	struct itemone *one = nemoitem_search_one(item, path);
	const char *str = one != NULL ? nemoitem_one_get_attr(one, name) : NULL;

	return str != NULL ? str : value;
}

static inline int nemoitem_box_get_count(struct itembox *box)
{
	return box->nones;
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

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
