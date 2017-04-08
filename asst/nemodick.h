#ifndef	__NEMO_DICK_H__
#define	__NEMO_DICK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

struct dickone {
	struct nemolist link;

	char *name;
	void *node;
};

struct nemodick {
	struct nemolist list;
};

extern struct nemodick *nemodick_create(void);
extern void nemodick_destroy(struct nemodick *dick);

extern void nemodick_insert(struct nemodick *dick, void *node, const char *name);
extern void nemodick_insert_format(struct nemodick *dick, void *node, const char *fmt, ...);
extern void nemodick_remove(struct nemodick *dick, const char *name);
extern void nemodick_remove_format(struct nemodick *dick, const char *fmt, ...);
extern void *nemodick_search(struct nemodick *dick, const char *name);
extern void *nemodick_search_format(struct nemodick *dick, const char *fmt, ...);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
