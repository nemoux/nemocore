#ifndef	__NEMOSHOW_COLOR_H__
#define	__NEMOSHOW_COLOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define	NEMOSHOW_COLOR_TO_UINT32(b)	\
	((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | ((uint32_t)b[3] << 0)

#define	NEMOSHOW_COLOR_UINT32_R(u)	\
	(uint8_t)((u >> 24) & 0xff)
#define	NEMOSHOW_COLOR_UINT32_G(u)	\
	(uint8_t)((u >> 16) & 0xff)
#define	NEMOSHOW_COLOR_UINT32_B(u)	\
	(uint8_t)((u >> 8) & 0xff)
#define	NEMOSHOW_COLOR_UINT32_A(u)	\
	(uint8_t)((u >> 0) & 0xff)

extern uint32_t nemoshow_color_parse(const char *value);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
