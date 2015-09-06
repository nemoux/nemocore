#ifndef __NEMOMOSI_FLIP_H__
#define __NEMOMOSI_FLIP_H__

#include <stdint.h>

struct nemomosi;

extern void nemomosi_flip_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t s0, uint32_t s1, uint32_t d0, uint32_t d1);

#endif
