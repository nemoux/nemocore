#ifndef	__NEMO_MISC_H__
#define	__NEMO_MISC_H__

#include <stdio.h>
#include <stdint.h>

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

#define	ARRAY_APPEND(a, s, n, v)	\
	if (n >= s) {	\
		a = (__typeof__(a))realloc(a, sizeof(a[0]) * s * 2);	\
		s = s * 2;	\
	}	\
	a[n++] = v

#define	ARRAY_APPEND_BUFFER(a, s, n, b, l)	\
	if (n + l >= s) {	\
		a = (__typeof__(a))realloc(a, sizeof(a[0]) * (s + l));	\
		s = s + l;	\
	}	\
	memcpy(&a[n], b, sizeof(a[0]) * l);	\
	n = n + l

#define	ARRAY_APPEND_STRING(a, s, n, b, l)	\
	if (n + l + 1 >= s) {	\
		a = (__typeof__(a))realloc(a, sizeof(a[0]) * (s + l + 1));	\
		s = s + l + 1;	\
	}	\
	memcpy(&a[n], b, sizeof(a[0]) * l);	\
	a[n + l] = '\0';	\
	n = n + l;

#define	ARRAY_REMOVE(a, n, i)	\
	if (i < n - 1) {	\
		memcpy(a + i, a + i + 1, sizeof(a[0]) * (n - i - 1));	\
	}	\
	n--

#define	ARRAY_BSEARCH(a, n, t, c)	\
	bsearch(t, a, n, sizeof(a[0]), c)

#define	ARRAY_QSORT(a, n, c)	\
	qsort(a, n, sizeof(a[0]), c)

extern void debug_show_backtrace(void);

extern uint32_t time_current_msecs(void);

extern int random_get_int(int min, int max);
extern double random_get_double(double min, double max);

#endif
