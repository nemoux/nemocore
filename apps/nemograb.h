#ifndef	__NEMOUX_GRAB_H__
#define	__NEMOUX_GRAB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>
#include <nemolistener.h>

#include <talehelper.h>
#include <nemomisc.h>

struct nemograb {
	struct talegrab base;

	struct nemolistener destroy_listener;

	double x, y;
	double dx, dy;

	void *data;
	uint32_t tag;
};

extern struct nemograb *nemograb_create(struct nemotale *tale, struct taleevent *event, nemotale_dispatch_grab_t dispatch);
extern void nemograb_destroy(struct nemograb *grab);

extern void nemograb_check_signal(struct nemograb *grab, struct nemosignal *signal);

static inline void nemograb_set_userdata(struct nemograb *grab, void *data)
{
	grab->data = data;
}

static inline void *nemograb_get_userdata(struct nemograb *grab)
{
	return grab->data;
}

static inline void nemograb_set_tag(struct nemograb *grab, uint32_t tag)
{
	grab->tag = tag;
}

static inline uint32_t nemograb_get_tag(struct nemograb *grab)
{
	return grab->tag;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
