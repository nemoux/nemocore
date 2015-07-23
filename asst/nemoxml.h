#ifndef	__NEMO_XML_H__
#define	__NEMO_XML_H__

#include <nemolist.h>

#define	NEMOXML_NAME_MAX			(32)
#define	NEMOXML_PATH_MAX			(128)
#define	NEMOXML_ATTRS_MAX			(32)

struct xmlnode {
	struct xmlnode *parent;
	struct nemolist link;
	struct nemolist children;
	struct nemolist nodelink;

	char name[NEMOXML_NAME_MAX];
	char path[NEMOXML_PATH_MAX];
	char *attrs[NEMOXML_ATTRS_MAX * 2];
	int nattrs;
	char *contents;
	int ncontents, scontents;

	int level;
};

struct nemoxml {
	struct nemolist children;
	struct nemolist nodes;
};

extern struct nemoxml *nemoxml_create(void);
extern void nemoxml_destroy(struct nemoxml *xml);

extern void nemoxml_update(struct nemoxml *xml);

extern struct xmlnode *nemoxml_create_node(void);
extern void nemoxml_destroy_node(struct xmlnode *node);

extern int nemoxml_load_file(struct nemoxml *xml, const char *filepath);
extern int nemoxml_load_data(struct nemoxml *xml, const char *buffer, int length);

extern void nemoxml_node_set_name(struct xmlnode *node, const char *name);
extern int nemoxml_node_set_attr(struct xmlnode *node, const char *attr, const char *value);
extern char *nemoxml_node_get_attr(struct xmlnode *node, const char *attr);
extern void nemoxml_node_set_contents(struct xmlnode *node, const char *contents, int length);

extern void nemoxml_link_node(struct nemoxml *xml, struct xmlnode *node);
extern void nemoxml_unlink_node(struct nemoxml *xml, struct xmlnode *node);
extern void nemoxml_node_link_node(struct xmlnode *node, struct xmlnode *child);
extern void nemoxml_node_unlink_node(struct xmlnode *node, struct xmlnode *child);

extern int nemoxml_select_node(struct nemoxml *xml, struct xmlnode **nodes, int length, const char *name);
extern int nemoxml_select_node_by_path(struct nemoxml *xml, struct xmlnode **nodes, int length, const char *path);
extern int nemoxml_select_node_by_attr(struct nemoxml *xml, struct xmlnode **nodes, int length, const char *name, const char *attr, const char *value);

#endif
