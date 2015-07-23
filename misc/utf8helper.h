#ifndef	__UTF8_HELPER_H__
#define	__UTF8_HELPER_H__

#include <stdint.h>

extern int utf8_from_ucs4(char *dst, const uint32_t *src, size_t size);
extern const char *utf8_prev_char(const char *s, const char *p);
extern const char *utf8_next_char(const char *p);
extern int utf8_length(const char *p);

#endif
