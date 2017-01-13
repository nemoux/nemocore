#ifndef __NEMOPLAY_BOX_H__
#define __NEMOPLAY_BOX_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct playone;

struct playbox {
	struct playone **ones;
	int nones, sones;
};

extern struct playbox *nemoplay_box_create(int size);
extern void nemoplay_box_destroy(struct playbox *box);

extern int nemoplay_box_insert_one(struct playbox *box, struct playone *one);

static inline struct playone *nemoplay_box_get_one(struct playbox *box, int index)
{
	return box->ones[index];
}

static inline void nemoplay_box_set_one(struct playbox *box, int index, struct playone *one)
{
	box->ones[index] = one;
}

static inline int nemoplay_box_get_count(struct playbox *box)
{
	return box->nones;
}

static inline void nemoplay_box_set_count(struct playbox *box, int count)
{
	box->nones = count;
}

static inline int nemoplay_box_get_size(struct playbox *box)
{
	return box->sones;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
