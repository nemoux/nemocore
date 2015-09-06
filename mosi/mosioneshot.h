#ifndef __NEMOMOSI_ONESHOT_H__
#define __NEMOMOSI_ONESHOT_H__

#include <stdint.h>

struct nemomosi;

extern void nemomosi_oneshot_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t s, uint32_t d);

#endif
