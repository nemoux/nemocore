#ifndef	__NEMOMOTE_COLOR_BUILDER_H__
#define	__NEMOMOTE_COLOR_BUILDER_H__

extern int nemomote_color_update(struct nemomote *mote, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin);
extern int nemomote_color_set(struct nemomote *mote, int base, int count, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin);

#endif
