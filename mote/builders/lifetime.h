#ifndef __NEMOMOTE_LIFETIME_BUILDER_H__
#define	__NEMOMOTE_LIFETIME_BUILDER_H__

extern int nemomote_lifetime_update(struct nemomote *mote, double max, double min);
extern int nemomote_lifetime_set(struct nemomote *mote, int base, int count, double max, double min);

#endif
