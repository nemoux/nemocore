#ifndef __NEMOMOSI_CROSS_H__
#define __NEMOMOSI_CROSS_H__

#include <stdint.h>

struct nemomosi;

extern void nemomosi_cross_dispatch(struct nemomosi *mosi, uint32_t msecs, uint32_t x, uint32_t y, uint32_t s0, uint32_t d0, uint32_t d1, double t0, double t1);

#endif
