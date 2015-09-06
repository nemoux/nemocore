#ifndef __NEMOMOSI_WAVE_H__
#define __NEMOMOSI_WAVE_H__

#include <stdint.h>

struct nemomosi;

extern void nemomosi_wave_dispatch(struct nemomosi *mosi, uint32_t msecs, double x, double y, double r, uint32_t s0, uint32_t s1, uint32_t d0, uint32_t d1);

#endif
