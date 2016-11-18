#ifndef __NEMOTILE_HELPER_HPP__
#define __NEMOTILE_HELPER_HPP__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct tileone;

extern struct tileone *nemotile_one_create_polygon(const char *filepath, const char *basepath);
extern struct tileone *nemotile_one_create_polyline(const char *filepath, const char *basepath);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
