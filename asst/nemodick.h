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

extern void nemodick_insert(struct nemodick *dick, const char *name, void *node);
extern void nemodick_remove(struct nemodick *dick, const char *name);
extern void *nemodick_search(struct nemodick *dick, const char *name);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
