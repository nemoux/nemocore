#ifndef	__NEMO_DATASELECTION_H__
#define	__NEMO_DATASELECTION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemoseat;
struct nemodatasource;

extern void dataselection_set_selection(struct nemoseat *seat, struct nemodatasource *source, uint32_t serial);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
