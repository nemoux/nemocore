#ifndef __NEMOMOTE_MASS_BUILDER_H__
#define	__NEMOMOTE_MASS_BUILDER_H__

extern int nemomote_mass_update(struct nemomote *mote, double max, double min);
extern int nemomote_mass_set(struct nemomote *mote, int base, int count, double max, double min);

#endif
