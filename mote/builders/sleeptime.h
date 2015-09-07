#ifndef __NEMOMOTE_SLEEPTIME_BUILDER_H__
#define	__NEMOMOTE_SLEEPTIME_BUILDER_H__

extern int nemomote_sleeptime_update(struct nemomote *mote, double max, double min);
extern int nemomote_sleeptime_set(struct nemomote *mote, int base, int count, double max, double min);

#endif
