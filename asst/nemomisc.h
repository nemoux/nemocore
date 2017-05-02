#ifndef	__NEMO_MISC_H__
#define	__NEMO_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdio.h>
#include <stdint.h>

#ifndef MIN
#	define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#	define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN3
# define MIN3(x, y, z)		((x) < (y) ? ((x) < (z) ? (x) : (z)) : ((y) < (z) ? (y) : (z)))
#endif

#ifndef MAX3
# define MAX3(x, y, z)		((x) > (y) ? ((x) > (z) ? (x) : (z)) : ((y) > (z) ? (y) : (z)))
#endif

#ifndef CLAMP
#	define CLAMP(x, a, b)		(MIN(MAX(x, a), b))
#endif

#ifndef MINMAX
#	define MINMAX(x, y, z)	(MIN(MAX(x, y), z))
#endif

#ifndef SQUARE
#	define SQUARE(x)				((x) * (x))
#endif

#ifndef ALIGN
#	define ALIGN(x, n)			(((x - 1) / n + 1) * n)
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
#	define	NEMO_CHECK(check, fmt, a...) if ((check) != 0) fprintf(stderr, "NEMO: (%s:%d) " fmt, __FUNCTION__, __LINE__, ##a)
#else
#	define	NEMO_ERROR(fmt, a...)
#	define	NEMO_DEBUG(fmt, a...)
#	define	NEMO_TRACE()
#	define	NEMO_CHECK(check, fmt, a...)
#endif

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a)[0])

extern uint32_t time_current_msecs(void);
extern uint64_t time_current_nsecs(void);
extern double time_current_secs(void);
extern void time_get_string(const char *fmt, char *buffer, int length);

extern int random_get_integer(int min, int max);
extern double random_get_double(double min, double max);

extern int os_socketpair_cloexec(int domain, int type, int protocol, int *sv);

extern int os_epoll_create_cloexec(void);
extern int os_epoll_add_fd(int efd, int fd, uint32_t events, void *data);
extern int os_epoll_del_fd(int efd, int fd);
extern int os_epoll_set_fd(int efd, int fd, uint32_t events, void *data);

extern int os_timerfd_create_cloexec(void);
extern int os_timerfd_set_timeout(int tfd, uint32_t secs, uint32_t nsecs);

extern int os_file_create_temp(const char *fmt, ...);
extern int os_file_is_exist(const char *path);
extern int os_file_is_directory(const char *path);
extern int os_file_is_regular(const char *path);
extern int os_file_load(const char *path, char **buffer);
extern int os_file_save(const char *path, char *buffer, int size);
extern uint32_t os_file_execute(const char *path, char *const argv[], char *const envp[]);

extern int os_fd_set_nonblocking_mode(int fd);
extern int os_fd_put_nonblocking_mode(int fd);

extern int os_sched_set_affinity(pid_t pid, uint32_t cpuid);

extern char *env_get_string(const char *name, char *value);
extern double env_get_double(const char *name, double value);
extern int env_get_integer(const char *name, int value);
extern void env_set_string(const char *name, const char *value);
extern void env_set_double(const char *name, double value);
extern void env_set_integer(const char *name, int value);
extern void env_set_format(const char *name, const char *fmt, ...);
extern void env_put_value(const char *name);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
