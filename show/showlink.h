#ifndef	__NEMOSHOW_LINK_H__
#define	__NEMOSHOW_LINK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <showone.h>

struct showlink {
	struct showone base;

	struct showone *head;
	struct showone *tail;
};

#define NEMOSHOW_LINK(one)					((struct showlink *)container_of(one, struct showlink, base))
#define NEMOSHOW_LINK_AT(one, at)		(NEMOSHOW_LINK(one)->at)

extern struct showone *nemoshow_link_create(void);
extern void nemoshow_link_destroy(struct showone *one);

extern int nemoshow_link_arrange(struct nemoshow *show, struct showone *one);
extern int nemoshow_link_update(struct nemoshow *show, struct showone *one);

static inline void nemoshow_link_set_head(struct showone *one, struct showone *head)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->head = head;
}

static inline void nemoshow_link_set_tail(struct showone *one, struct showone *tail)
{
	struct showlink *link = NEMOSHOW_LINK(one);

	link->tail = tail;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
