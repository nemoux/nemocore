#ifndef	__NEMO_DATASELECTION_H__
#define	__NEMO_DATASELECTION_H__

#include <stdint.h>

struct nemoseat;
struct nemodatasource;

extern void dataselection_set_selection(struct nemoseat *seat, struct nemodatasource *source, uint32_t serial);

#endif
