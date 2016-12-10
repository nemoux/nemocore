#ifndef	__CODE_HELPER_H__
#define	__CODE_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern uint64_t crc32_from_string(const char *str);

extern int base64_encode(char *input, int length, char *output);
extern int base64_decode(char *input, int length, char *output);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
