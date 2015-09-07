#ifndef __NEMOMOTE_POSITION_BUILDER_H__
#define	__NEMOMOTE_POSITION_BUILDER_H__

extern int nemomote_position_update(struct nemomote *mote, struct nemozone *zone);
extern int nemomote_position_set(struct nemomote *mote, uint32_t type, struct nemozone *zone);

#endif
