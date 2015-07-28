#ifndef	__NEMOSHOW_SEQUENCE_H__
#define	__NEMOSHOW_SEQUENCE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoattr.h>

#include <showone.h>

#define	NEMOSHOW_SEQUENCE_TYPE_MAX			(32)
#define	NEMOSHOW_SEQUENCE_SET_ATTR_MAX	(8)

struct nemoshow;

struct showset {
	struct showone base;

	char src[NEMOSHOW_ID_MAX];

	double sattrs[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	struct nemoattr *tattrs[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	struct nemoattr *eattrs[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	int nattrs;
};

struct showframe {
	struct showone base;

	struct showone **sets;
	int nsets, ssets;

	double t;
};

struct showsequence {
	struct showone base;

	struct showone **frames;
	int nframes, sframes;

	double t;
	int iframe;
};

#define NEMOSHOW_SEQUENCE(one)		((struct showsequence *)container_of(one, struct showsequence, base))
#define NEMOSHOW_FRAME(one)				((struct showframe *)container_of(one, struct showframe, base))
#define NEMOSHOW_SET(one)					((struct showset *)container_of(one, struct showset, base))

extern struct showone *nemoshow_sequence_create(void);
extern void nemoshow_sequence_destroy(struct showone *one);

extern void nemoshow_sequence_prepare(struct showone *one);
extern void nemoshow_sequence_update(struct showone *one, double t);

extern struct showone *nemoshow_sequence_create_frame(void);
extern void nemoshow_sequence_destroy_frame(struct showone *one);

extern struct showone *nemoshow_sequence_create_set(void);
extern void nemoshow_sequence_destroy_set(struct showone *one);

extern int nemoshow_sequence_arrange_set(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
