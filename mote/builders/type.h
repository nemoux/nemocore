#ifndef	__NEMOMOTE_TYPE_BUILDER_H__
#define	__NEMOMOTE_TYPE_BUILDER_H__

extern int nemomote_type_update(struct nemomote *mote, uint32_t type);
extern int nemomote_type_set(struct nemomote *mote, uint32_t type, uint32_t ntype);
extern int nemomote_type_set_one(struct nemomote *mote, int index, uint32_t type);

#endif
