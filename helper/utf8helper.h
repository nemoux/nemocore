#ifndef	__UTF8_HELPER_H__
#define	__UTF8_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern int utf8_from_ucs4(char *dst, const uint32_t *src, size_t size);
extern const char *utf8_prev_char(const char *s, const char *p);
extern const char *utf8_next_char(const char *p);
extern int utf8_length(const char *p);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
