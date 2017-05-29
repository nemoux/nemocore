#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <nemolog.h>
#include <nemomisc.h>

#ifdef NEMO_LOG_ON
static __thread uint64_t s_nsecs;

static int s_logfile = -1;
static int s_logtype = 0;

void __attribute__((constructor(101))) nemolog_initialize(void)
{
	s_nsecs = time_current_nsecs();
}

void __attribute__((destructor(101))) nemolog_finalize(void)
{
}

static int nemolog_prepare(void)
{
	if (getenv("NEMOLOG_SOCKET_PATH") != NULL)
		nemolog_open_socket(getenv("NEMOLOG_SOCKET_PATH"));
	else if (getenv("NEMOLOG_FILE_PATH") != NULL)
		nemolog_open_file(getenv("NEMOLOG_FILE_PATH"));
	else
		nemolog_set_file(2);

	return s_logfile;
}

static void nemolog_finish(void)
{
	nemolog_close_file();
}

int nemolog_open_file(const char *filepath)
{
	s_logfile = open(filepath, O_RDWR | O_CREAT | O_NONBLOCK, 0644);
	s_logtype = 0;

	return s_logfile;
}

void nemolog_close_file(void)
{
	close(s_logfile);

	s_logfile = -1;
}

void nemolog_set_file(int fd)
{
	os_fd_set_nonblocking_mode(fd);

	s_logfile = fd;
}

int nemolog_open_socket(const char *socketpath)
{
	struct sockaddr_un addr;
	socklen_t size, namesize;
	int soc;

	soc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (soc < 0)
		return -1;

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketpath);
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (connect(soc, (struct sockaddr *)&addr, size) < 0)
		goto err1;

	os_fd_set_nonblocking_mode(soc);

	s_logfile = soc;
	s_logtype = 1;

	return soc;

err1:
	close(soc);

	return -1;
}

static int nemolog_write(const char *syntax, const char *tag, double secs, const char *fmt, va_list vargs)
{
	char msg[1024];

	snprintf(msg, sizeof(msg), syntax, tag, secs);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, vargs);

	if (s_logtype == 1)
		return send(s_logfile, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);

	return write(s_logfile, msg, strlen(msg));
}

int nemolog_message(const char *tag, const char *fmt, ...)
{
	uint64_t nsecs = time_current_nsecs();
	va_list vargs;
	int r;

	if (s_logfile < 0 && nemolog_prepare() < 0)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;33m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - s_nsecs) / 1000000000.0f, fmt, vargs);
	va_end(vargs);

	if (r <= 0)
		s_logfile = -1;

	return r;
}

int nemolog_warning(const char *tag, const char *fmt, ...)
{
	uint64_t nsecs = time_current_nsecs();
	va_list vargs;
	int r;

	if (s_logfile < 0 && nemolog_prepare() < 0)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;30m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - s_nsecs) / 1000000000.0f, fmt, vargs);
	va_end(vargs);

	if (r <= 0)
		s_logfile = -1;

	return r;
}

int nemolog_error(const char *tag, const char *fmt, ...)
{
	uint64_t nsecs = time_current_nsecs();
	va_list vargs;
	int r;

	if (s_logfile < 0 && nemolog_prepare() < 0)
		return 0;

	va_start(vargs, fmt);
	r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;31m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - s_nsecs) / 1000000000.0f, fmt, vargs);
	va_end(vargs);

	if (r <= 0)
		s_logfile = -1;

	return r;
}

int nemolog_check(int check, const char *tag, const char *fmt, ...)
{
	if (check != 0) {
		uint64_t nsecs = time_current_nsecs();
		va_list vargs;
		int r;

		if (s_logfile < 0 && nemolog_prepare() < 0)
			return 0;

		va_start(vargs, fmt);
		r = nemolog_write("\e[32;1mNEMO:\e[m \e[1;34m[%s] (%.3f)\e[0m ", tag, (double)(nsecs - s_nsecs) / 1000000000.0f, fmt, vargs);
		va_end(vargs);

		if (r <= 0)
			s_logfile = -1;

		return r;
	}

	return 0;
}

void nemolog_checkpoint(void)
{
	s_nsecs = time_current_nsecs();
}

int nemolog_event(const char *tag, const char *fmt, ...)
{
	time_t ttime;
	struct tm *stime;
	char times[128];
	char msg[1024];
	va_list vargs;
	int r;

	if (s_logfile < 0 && nemolog_prepare() < 0)
		return 0;

	time(&ttime);
	stime = localtime(&ttime);
	strftime(times, sizeof(times), "%Y:%m:%d-%H:%M:%S", stime);

	va_start(vargs, fmt);

	snprintf(msg, sizeof(msg), "NEMO-EVENT: [%s] (%s) ", tag, times);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, vargs);

	if (s_logtype == 1)
		r = send(s_logfile, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);
	else
		r = write(s_logfile, msg, strlen(msg));

	va_end(vargs);

	if (r <= 0)
		s_logfile = -1;

	return r;
}

#endif
