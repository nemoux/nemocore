#ifndef __NEMOMOTE_VELOCITY_BUILDER_H__
#define	__NEMOMOTE_VELOCITY_BUILDER_H__

extern int nemomote_velocity_update(struct nemomote *mote, struct nemozone *zone);
extern int nemomote_velocity_set(struct nemomote *mote, int base, int count, struct nemozone *zone);

#endif
