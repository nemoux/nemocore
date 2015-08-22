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
#define	NEMOSHOW_SEQUENCE_SET_ATTR_MAX	(16)

typedef enum {
	NEMOSHOW_PATH_X_FOLLOW = 0,
	NEMOSHOW_PATH_Y_FOLLOW = 1,
	NEMOSHOW_PATH_R_FOLLOW = 2,
	NEMOSHOW_PATH_LAST_FOLLOW
} NemoShowFollowElement;

struct nemoshow;
struct showone;

struct showfollow {
	struct showone base;

	struct showone *src;
	struct nemoattr *attr;

	struct showone *path;
	double from, to;
	int element;
};

struct showset {
	struct showone base;

	struct showone *src;

	double sattrs[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	struct nemoattr *tattrs[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	struct nemoattr *eattrs[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	uint32_t dirties[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	int types[NEMOSHOW_SEQUENCE_SET_ATTR_MAX];
	int nattrs;
};

struct showframe {
	struct showone base;

	double t;
};

struct showsequence {
	struct showone base;

	double t;
	int iframe;
};

#define NEMOSHOW_SEQUENCE(one)					((struct showsequence *)container_of(one, struct showsequence, base))
#define NEMOSHOW_SEQUENCE_AT(one, at)		(NEMOSHOW_SEQUENCE(one)->at)
#define NEMOSHOW_FRAME(one)							((struct showframe *)container_of(one, struct showframe, base))
#define NEMOSHOW_FRAME_AT(one, at)			(NEMOSHOW_FRAME(one)->at)
#define NEMOSHOW_SET(one)								((struct showset *)container_of(one, struct showset, base))
#define NEMOSHOW_SET_AT(one, at)				(NEMOSHOW_SET(one)->at)
#define NEMOSHOW_FOLLOW(one)						((struct showfollow *)container_of(one, struct showfollow, base))
#define NEMOSHOW_FOLLOW_AT(one, at)			(NEMOSHOW_FOLLOW(one)->at)

extern struct showone *nemoshow_sequence_create(void);
extern void nemoshow_sequence_destroy(struct showone *one);

extern int nemoshow_sequence_update(struct nemoshow *show, struct showone *one);

extern void nemoshow_sequence_prepare(struct showone *one, uint32_t serial);
extern void nemoshow_sequence_dispatch(struct showone *one, double t, uint32_t serial);

extern struct showone *nemoshow_sequence_create_frame(void);
extern void nemoshow_sequence_destroy_frame(struct showone *one);

extern int nemoshow_sequence_update_frame(struct nemoshow *show, struct showone *one);

extern int nemoshow_sequence_set_timing(struct showone *one, double t);

extern struct showone *nemoshow_sequence_create_set(void);
extern void nemoshow_sequence_destroy_set(struct showone *one);

extern int nemoshow_sequence_arrange_set(struct nemoshow *show, struct showone *one);
extern int nemoshow_sequence_update_set(struct nemoshow *show, struct showone *one);

extern int nemoshow_sequence_set_source(struct showone *one, struct showone *src);
extern int nemoshow_sequence_set_attr(struct showone *one, const char *name, const char *value);
extern int nemoshow_sequence_set_dattr(struct showone *one, const char *name, double value, uint32_t dirty);

extern struct showone *nemoshow_sequence_create_follow(void);
extern void nemoshow_sequence_destroy_follow(struct showone *one);

extern int nemoshow_sequence_arrange_follow(struct nemoshow *show, struct showone *one);
extern int nemoshow_sequence_update_follow(struct nemoshow *show, struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
