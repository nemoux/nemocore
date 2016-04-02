#ifndef	__NEMO_MISC_H__
#define	__NEMO_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdio.h>
#include <stdint.h>

#ifndef MIN
#	define MIN(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef MAX
#	define MAX(x,y) (((x) > (y)) ? (x) : (y))
#endif

#ifndef MINMAX
# define MINMAX(x,y,z)	(MIN(MAX(x,y), z))
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
extern uint64_t time_current_nsecs(void);

extern int random_get_int(int min, int max);
extern double random_get_double(double min, double max);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
