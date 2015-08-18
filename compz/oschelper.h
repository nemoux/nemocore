#ifndef	__OSC_HELPER_H__
#define	__OSC_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#define	OSC_TRUE_TAG					('T')
#define	OSC_FALSE_TAG					('F')
#define	OSC_NIL_TAG						('N')
#define	OSC_INFINITUM_TAG			('I')
#define	OSC_INT32_TAG					('i')
#define	OSC_FLOAT_TAG					('f')
#define	OSC_CHAR_TAG					('c')
#define	OSC_RGBA_COLOR_TAG		('r')
#define	OSC_MIDI_MESSAGE_TAG	('m')
#define	OSC_INT64_TAG					('h')
#define	OSC_TIME_TAG					('t')
#define	OSC_DOUBLE_TAG				('d')
#define	OSC_STRING_TAG				('s')
#define	OSC_SYMBOL_TAG				('S')
#define	OSC_BLOB_TAG					('b')

static inline const char *osc_find_str4(const char *p)
{
	if (p[0] == '\0')
		return p + 4;

	for (p += 3; *p != '\0'; p += 4);

	return p + 1;
}

static inline const char *osc_find_str4_end(const char *p, const char *end)
{
	if (p >= end)
		return NULL;

	if (p[0] == '\0')
		return p + 4;

	for (p += 3, end -= 1; p < end && *p != '\0'; p += 4);

	if (*p != '\0')
		return NULL;

	return p + 1;
}

static inline uint32_t osc_round_up(uint32_t x)
{
	uint32_t remainder = x & 0x3;

	if (remainder != 0)
		return x + (4 - remainder);

	return x;
}

static inline int32_t osc_int32(const char *p)
{
	union {
		int32_t i;
		char c[4];
	} u;

	u.c[0] = p[3];
	u.c[1] = p[2];
	u.c[2] = p[1];
	u.c[3] = p[0];

	return u.i;
}

static inline uint32_t osc_uint32(const char *p)
{
	union {
		uint32_t i;
		char c[4];
	} u;

	u.c[0] = p[3];
	u.c[1] = p[2];
	u.c[2] = p[1];
	u.c[3] = p[0];

	return u.i;
}

static inline int32_t osc_int64(const char *p)
{
	union {
		int64_t i;
		char c[8];
	} u;

	u.c[0] = p[7];
	u.c[1] = p[6];
	u.c[2] = p[5];
	u.c[3] = p[4];
	u.c[4] = p[3];
	u.c[5] = p[2];
	u.c[6] = p[1];
	u.c[7] = p[0];

	return u.i;
}

static inline uint32_t osc_uint64(const char *p)
{
	union {
		uint64_t i;
		char c[8];
	} u;

	u.c[0] = p[7];
	u.c[1] = p[6];
	u.c[2] = p[5];
	u.c[3] = p[4];
	u.c[4] = p[3];
	u.c[5] = p[2];
	u.c[6] = p[1];
	u.c[7] = p[0];

	return u.i;
}

static inline float osc_float(const char *p)
{
	union {
		float f;
		char c[4];
	} u;

	u.c[0] = p[3];
	u.c[1] = p[2];
	u.c[2] = p[1];
	u.c[3] = p[0];

	return u.f;
}

static inline double osc_double(const char *p)
{
	union {
		double d;
		char c[8];
	} u;

	u.c[0] = p[7];
	u.c[1] = p[6];
	u.c[2] = p[5];
	u.c[3] = p[4];
	u.c[4] = p[3];
	u.c[5] = p[2];
	u.c[6] = p[1];
	u.c[7] = p[0];

	return u.d;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
