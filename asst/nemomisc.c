#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <math.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/stat.h>

#include <nemomisc.h>

uint32_t time_current_msecs(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint64_t time_current_nsecs(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

double time_current_secs(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0f;
}

void time_get_string(const char *fmt, char *buffer, int length)
{
	time_t ttime;
	struct tm *stime;

	time(&ttime);
	stime = localtime(&ttime);
	strftime(buffer, length, fmt, stime);
}

int random_get_integer(int min, int max)
{
	if (min == max)
		return min;

	return random() % (max - min + 1) + min;
}

double random_get_double(double min, double max)
{
	if (min == max)
		return min;

	return ((double)random() / (double)RAND_MAX) * (max - min) + min;
}

static int os_set_cloexec_or_close(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		goto err;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto err;

	return fd;

err:
	close(fd);

	return -1;
}

int os_socketpair_cloexec(int domain, int type, int protocol, int *sv)
{
	int ret;

#ifdef SOCK_CLOEXEC
	ret = socketpair(domain, type | SOCK_CLOEXEC, protocol, sv);
	if (ret == 0 || errno != EINVAL)
		return ret;
#endif

	ret = socketpair(domain, type, protocol, sv);
	if (ret < 0)
		return ret;

	sv[0] = os_set_cloexec_or_close(sv[0]);
	sv[1] = os_set_cloexec_or_close(sv[1]);

	if (sv[0] != -1 && sv[1] != -1)
		return 0;

	close(sv[0]);
	close(sv[1]);

	return -1;
}

int os_epoll_create_cloexec(void)
{
	int fd;

#ifdef EPOLL_CLOEXEC
	fd = epoll_create1(EPOLL_CLOEXEC);
	if (fd >= 0)
		return fd;
	if (errno != EINVAL)
		return -1;
#endif

	fd = epoll_create(1);
	return os_set_cloexec_or_close(fd);
}

int os_epoll_add_fd(int efd, int fd, uint32_t events, void *data)
{
	struct epoll_event ep;

	ep.events = events;
	ep.data.ptr = data;
	epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ep);

	return 0;
}

int os_epoll_del_fd(int efd, int fd)
{
	epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);

	return 0;
}

int os_epoll_set_fd(int efd, int fd, uint32_t events, void *data)
{
	struct epoll_event ep;

	ep.events = events;
	ep.data.ptr = data;
	epoll_ctl(efd, EPOLL_CTL_MOD, fd, &ep);

	return 0;
}

int os_timerfd_create_cloexec(void)
{
	return timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
}

int os_timerfd_set_timeout(int tfd, uint32_t secs, uint32_t nsecs)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = secs;
	its.it_value.tv_nsec = 0;

	return timerfd_settime(tfd, 0, &its, NULL);
}

int os_file_create_temp(const char *fmt, ...)
{
	va_list vargs;
	char *tmpname;
	int fd;

	va_start(vargs, fmt);
	vasprintf(&tmpname, fmt, vargs);
	va_end(vargs);

#ifdef HAVE_MKOSTEMP
	fd = mkostemp(tmpname, O_CLOEXEC);
	if (fd >= 0)
		unlink(tmpname);
#else
	fd = mkstemp(tmpname);
	if (fd >= 0) {
		fd = os_set_cloexec_or_close(fd);
		unlink(tmpname);
	}
#endif

	free(tmpname);

	return fd;
}

int os_file_is_exist(const char *path)
{
	return access(path, F_OK) == 0;
}

int os_file_is_directory(const char *path)
{
	struct stat st;

	stat(path, &st);

	return S_ISDIR(st.st_mode);
}

int os_file_is_regular(const char *path)
{
	struct stat st;

	stat(path, &st);

	return S_ISREG(st.st_mode);
}

int os_file_load(const char *path, char **buffer)
{
	FILE *fp;
	char *contents;
	int size;

	if (path == NULL)
		return -1;

	fp = fopen(path, "rt");
	if (fp == NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	if (size <= 0) {
		fclose(fp);
		return 0;
	}

	contents = malloc(sizeof(char) * (size + 1));
	if (contents == NULL) {
		fclose(fp);
		return -1;
	}
	memset(contents, 0, size + 1);

	fread(contents, sizeof(char), size, fp);

	fclose(fp);

	if (buffer != NULL)
		*buffer = contents;

	return size;
}

int os_file_save(const char *path, char *buffer, int size)
{
	FILE *fp;

	if (path == NULL)
		return -1;

	fp = fopen(path, "w");
	if (fp == NULL)
		return -1;

	fwrite(buffer, sizeof(char), size, fp);

	fclose(fp);

	return 0;
}

uint32_t os_file_execute(const char *path, char *const argv[], char *const envp[])
{
	sigset_t allsigs;
	pid_t pid;

	pid = fork();
	if (pid == -1)
		return 0;

	if (pid > 0)
		return pid;

	sigfillset(&allsigs);
	sigprocmask(SIG_UNBLOCK, &allsigs, NULL);

	if (seteuid(getuid()) == -1)
		return 0;

	if (setpgid(getpid(), getpid()) == -1)
		return 0;

	if (argv == NULL || argv[0] == NULL) {
		execl(path, path, NULL);
	} else if (envp == NULL || envp[0] == NULL) {
		execv(path, argv);
	} else {
		execve(path, argv, envp);
	}

	exit(EXIT_FAILURE);

	return 0;
}

int os_fd_set_nonblocking_mode(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int os_fd_put_nonblocking_mode(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);

	return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int os_sched_set_affinity(pid_t pid, uint32_t cpuid)
{
	cpu_set_t cset;

	CPU_ZERO(&cset);
	CPU_SET(cpuid, &cset);

	return sched_setaffinity(pid, sizeof(cpu_set_t), &cset);
}

char *env_get_string(const char *name, char *value)
{
	char *env = getenv(name);

	return env != NULL ? env : value;
}

double env_get_double(const char *name, double value)
{
	const char *env = getenv(name);

	return env != NULL ? strtod(env, NULL) : value;
}

int env_get_integer(const char *name, int value)
{
	const char *env = getenv(name);

	return env != NULL ? strtoul(env, NULL, 10) : value;
}

void env_set_string(const char *name, const char *value)
{
	setenv(name, value, 1);
}

void env_set_double(const char *name, double value)
{
	char *env;

	asprintf(&env, "%f", value);

	setenv(name, env, 1);

	free(env);
}

void env_set_integer(const char *name, int value)
{
	char *env;

	asprintf(&env, "%d", value);

	setenv(name, env, 1);

	free(env);
}

void env_set_format(const char *name, const char *fmt, ...)
{
	va_list vargs;
	char *env;

	va_start(vargs, fmt);
	vasprintf(&env, fmt, vargs);
	va_end(vargs);

	setenv(name, env, 1);

	free(env);
}

void env_put_value(const char *name)
{
	unsetenv(name);
}
