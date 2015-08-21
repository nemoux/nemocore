#ifndef	__NEMO_MISC_H__
#define	__NEMO_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#ifndef MIN
#	define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#	define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef offsetof
#	define offsetof(type, member) \
	((char *)&((type *)0)->member - (char *)0)
#endif

#ifndef container_of
#	define container_of(ptr, type, member) ({				\
		const __typeof__(((type *)0)->member) *__mptr = (ptr);	\
		(type *)((char *)__mptr - offsetof(type, member));})
#endif

#ifdef NEMO_DEBUG_ON
#	define	NEMO_ERROR(fmt, a...)	fprintf(stderr, "NEMO: (%s:%d) " fmt, __FUNCTION__, __LINE__, ##a)
#	define	NEMO_DEBUG(fmt, a...)	fprintf(stderr, "NEMO: (%s:%d) " fmt, __FUNCTION__, __LINE__, ##a)
#	define	NEMO_TRACE()					fprintf(stderr, "NEMO: (%s:%d)\n", __FUNCTION__, __LINE__)
#else
#	define	NEMO_ERROR(fmt, a...)
#	define	NEMO_DEBUG(fmt, a...)
#	define	NEMO_TRACE()
#endif

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a)[0])

extern void debug_show_backtrace(void);

extern uint32_t time_current_msecs(void);

extern int random_get_int(int min, int max);
extern double random_get_double(double min, double max);

static inline double point_get_distance(double x0, double y0, double x1, double y1)
{
	double dx = x1 - x0;
	double dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
