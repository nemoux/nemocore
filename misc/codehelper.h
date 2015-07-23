#ifndef	__CODE_HELPER_H__
#define	__CODE_HELPER_H__

#include <stdint.h>

extern uint64_t crc32_from_string(const char *str);

extern int base64_encode(char *input, int length, char *output);
extern int base64_decode(char *input, int length, char *output);

#endif
