#ifndef	__NEMO_ITEM_H__
#define	__NEMO_ITEM_H__

#define	NEMOITEM_ATTR_MAX			(16)

struct itemnode {
	char *name;
	char *attrs[NEMOITEM_ATTR_MAX * 2];
	int nattrs;
};

struct nemoitem {
	struct itemnode *nodes;
	int nnodes;
	int mnodes;
};

extern struct nemoitem *nemoitem_create(int mnodes);
extern void nemoitem_destroy(struct nemoitem *item);

extern int nemoitem_set(struct nemoitem *item, const char *name);
extern int nemoitem_get(struct nemoitem *item, const char *name, int index);

extern int nemoitem_get_ifone(struct nemoitem *item, const char *name, int index, const char *attr0, const char *value0);
extern int nemoitem_get_iftwo(struct nemoitem *item, const char *name, int index, const char *attr0, const char *value0, const char *attr1, const char *value1);

extern int nemoitem_set_attr(struct nemoitem *item, int index, const char *attr, const char *value);
extern char *nemoitem_get_attr(struct nemoitem *item, int index, const char *attr);
extern char *nemoitem_get_vattr(struct nemoitem *item, int index, const char *fmt, ...);

extern void nemoitem_dump(struct nemoitem *item, FILE *out);

static inline int nemoitem_set_attr_named(struct nemoitem *item, const char *name, const char *attr, const char *value)
{
	int index;

	index = nemoitem_get(item, name, 0);
	if (index >= 0) {
		return nemoitem_set_attr(item, index, attr, value);
	}

	return -1;
}

static inline char *nemoitem_get_attr_named(struct nemoitem *item, const char *name, const char *attr)
{
	int index;

	index = nemoitem_get(item, name, 0);
	if (index >= 0) {
		return nemoitem_get_attr(item, index, attr);
	}

	return NULL;
}

#endif
