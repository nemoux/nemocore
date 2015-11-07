#ifndef __COLOR_HELPER_H__
#define __COLOR_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemotoken.h>
#include <nemomisc.h>

#define	NEMOCOLOR_TO_UINT32(b)	\
	((uint32_t)b[3] << 24) | ((uint32_t)b[2] << 16) | ((uint32_t)b[1] << 8) | ((uint32_t)b[0] << 0)

#define	NEMOCOLOR_UINT32_A(u)	\
	(uint8_t)((u >> 24) & 0xff)
#define	NEMOCOLOR_UINT32_R(u)	\
	(uint8_t)((u >> 16) & 0xff)
#define	NEMOCOLOR_UINT32_G(u)	\
	(uint8_t)((u >> 8) & 0xff)
#define	NEMOCOLOR_UINT32_B(u)	\
	(uint8_t)((u >> 0) & 0xff)

#define	NEMOCOLOR_DOUBLE_A(u)	\
	(double)NEMOCOLOR_UINT32_A(u) / (double)255
#define	NEMOCOLOR_DOUBLE_R(u)	\
	(double)NEMOCOLOR_UINT32_R(u) / (double)255
#define	NEMOCOLOR_DOUBLE_G(u)	\
	(double)NEMOCOLOR_UINT32_G(u) / (double)255
#define	NEMOCOLOR_DOUBLE_B(u)	\
	(double)NEMOCOLOR_UINT32_B(u) / (double)255

extern uint32_t nemocolor_parse(const char *value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
