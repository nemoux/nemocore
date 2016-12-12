#ifndef __NEMOPLAY_BOX_H__
#define __NEMOPLAY_BOX_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct playqueue;
struct playone;

struct playbox {
	struct playone **ones;
	int count;
};

extern struct playbox *nemoplay_box_create_by_queue(struct playqueue *queue);
extern void nemoplay_box_destroy(struct playbox *box);

static inline struct playone *nemoplay_box_get_one(struct playbox *box, int index)
{
	return box->ones[index];
}

static inline int nemoplay_box_get_count(struct playbox *box)
{
	return box->count;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
