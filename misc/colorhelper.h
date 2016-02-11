#ifndef __COLOR_HELPER_H__
#define __COLOR_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotoken.h>
#include <nemomisc.h>

#define	COLOR_TO_UINT32(b)	\
	((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16) | ((uint32_t)b[1] << 8) | ((uint32_t)b[0] << 0)

#define	COLOR_UINT32_A(u)	\
	(uint8_t)((u >> 24) & 0xff)
#define	COLOR_UINT32_R(u)	\
	(uint8_t)((u >> 16) & 0xff)
#define	COLOR_UINT32_G(u)	\
	(uint8_t)((u >> 8) & 0xff)
#define	COLOR_UINT32_B(u)	\
	(uint8_t)((u >> 0) & 0xff)

#define	COLOR_DOUBLE_A(u)	\
	(double)COLOR_UINT32_A(u) / (double)255
#define	COLOR_DOUBLE_R(u)	\
	(double)COLOR_UINT32_R(u) / (double)255
#define	COLOR_DOUBLE_G(u)	\
	(double)COLOR_UINT32_G(u) / (double)255
#define	COLOR_DOUBLE_B(u)	\
	(double)COLOR_UINT32_B(u) / (double)255

extern uint32_t color_parse(const char *value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
