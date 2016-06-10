#ifndef __NAMESPACE_HELPER_H__
#define __NAMESPACE_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int namespace_has_prefix(const char *ns, const char *ps);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
