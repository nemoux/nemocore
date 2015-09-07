#ifndef __NEMOMOTE_SLEEPTIME_BUILDER_H__
#define	__NEMOMOTE_SLEEPTIME_BUILDER_H__

extern int nemomote_sleeptime_update(struct nemomote *mote, double max, double min);
extern int nemomote_sleeptime_set(struct nemomote *mote, uint32_t type, double max, double min);

#endif
