#ifndef __NAMESPACE_HELPER_H__
#define __NAMESPACE_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int namespace_has_prefix(const char *ns, const char *ps);
extern int namespace_has_prefix_format(const char *ns, const char *fmt, ...);
extern int namespace_has_regex(const char *ns, const char *expr);
extern int namespace_has_regex_format(const char *ns, const char *fmt, ...);

extern int namespace_get_count(const char *ns);
extern int namespace_get_path(const char *ns, char *ps, int index);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
