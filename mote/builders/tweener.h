#ifndef	__NEMOMOTE_TWEENER_BUILDER_H__
#define	__NEMOMOTE_TWEENER_BUILDER_H__

extern int nemomote_tweener_update_position(struct nemomote *mote, uint32_t type, double x, double y);
extern int nemomote_tweener_update_color(struct nemomote *mote, uint32_t type, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin);
extern int nemomote_tweener_update_mass(struct nemomote *mote, uint32_t type, double max, double min);
extern int nemomote_tweener_update_duration(struct nemomote *mote, uint32_t type, double max, double min);

extern int nemomote_tweener_set_position(struct nemomote *mote, uint32_t type, double x, double y);
extern int nemomote_tweener_set_color(struct nemomote *mote, uint32_t type, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin);
extern int nemomote_tweener_set_mass(struct nemomote *mote, uint32_t type, double max, double min);
extern int nemomote_tweener_set_duration(struct nemomote *mote, uint32_t type, double max, double min);

extern int nemomote_tweener_set_one_position(struct nemomote *mote, int index, double x, double y);
extern int nemomote_tweener_set_one_color(struct nemomote *mote, int index, double rmax, double rmin, double gmax, double gmin, double bmax, double bmin, double amax, double amin);
extern int nemomote_tweener_set_one_mass(struct nemomote *mote, int index, double max, double min);
extern int nemomote_tweener_set_one_duration(struct nemomote *mote, int index, double max, double min);

#endif
