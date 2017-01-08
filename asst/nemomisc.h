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
#	define CLAMP(x, a, b)	(MIN(MAX(x, a), b))
#endif

#ifndef MINMAX
#	define MINMAX(x, y, z)	(MIN(MAX(x, y), z))
#endif

#ifndef SQUARE
#	define SQUARE(x)			((x) * (x))
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

extern int random_get_int(int min, int max);
extern double random_get_double(double min, double max);

extern int os_socketpair_cloexec(int domain, int type, int protocol, int *sv);
extern int os_epoll_create_cloexec(void);
extern int os_epoll_add_fd(int efd, int fd, uint32_t events, void *data);
extern int os_epoll_del_fd(int efd, int fd);
extern int os_epoll_set_fd(int efd, int fd, uint32_t events, void *data);
extern int os_timerfd_create_cloexec(void);
extern int os_timerfd_set_timeout(int tfd, uint32_t secs, uint32_t nsecs);
extern int os_create_anonymous_file(off_t size);

extern int os_exist_path(const char *path);
extern int os_check_is_directory(const char *path);
extern int os_check_is_file(const char *path);
extern int os_load_path(const char *path, char **buffer, int *size);
extern int os_save_path(const char *path, char *buffer, int size);
extern uint32_t os_execute_path(const char *path, char *const argv[], char *const envp[]);

extern int os_append_path(char *path, const char *name);
extern const char *os_get_file_extension(const char *name);
extern int os_has_file_extension(const char *name, const char *ext);
extern int os_has_file_extensions(const char *name, ...);
extern char *os_get_file_path(const char *name);
extern char *os_get_file_name(const char *path);

extern int os_set_nonblocking_mode(int fd);

extern int os_sched_set_affinity(pid_t pid, uint32_t cpuid);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
