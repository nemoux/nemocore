#ifndef __MESH_HELPER_H__
#define __MESH_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int mesh_load_triangles(const char *filepath, const char *basepath, float **vertices, float **normals, float **texcoords, float **colors);
extern int mesh_load_lines(const char *filepath, const char *basepath, float **vertices, float **normals, float **colors);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
