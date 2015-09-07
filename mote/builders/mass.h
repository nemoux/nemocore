#ifndef __NEMOMOTE_MASS_BUILDER_H__
#define	__NEMOMOTE_MASS_BUILDER_H__

extern int nemomote_mass_update(struct nemomote *mote, double max, double min);
extern int nemomote_mass_set(struct nemomote *mote, uint32_t type, double max, double min);

#endif
