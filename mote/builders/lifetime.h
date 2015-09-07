#ifndef __NEMOMOTE_LIFETIME_BUILDER_H__
#define	__NEMOMOTE_LIFETIME_BUILDER_H__

extern int nemomote_lifetime_update(struct nemomote *mote, double max, double min);
extern int nemomote_lifetime_set(struct nemomote *mote, uint32_t type, double max, double min);

#endif
