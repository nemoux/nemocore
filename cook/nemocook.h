#ifndef __NEMOCOOK_H__
#define __NEMOCOOK_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <cookshader.h>
#include <cooktex.h>
#include <cookpoly.h>
#include <cooktrans.h>
#include <cookegl.h>
#include <cookfbo.h>
#include <cookone.h>
#include <cookstate.h>

extern void nemocook_draw_arrays(int mode, int ncounts, uint32_t *counts);
extern void nemocook_draw_texture(uint32_t tex, float x0, float y0, float x1, float y1);

extern void nemocook_clear_color_buffer(float r, float g, float b, float a);
extern void nemocook_clear_depth_buffer(float d);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
