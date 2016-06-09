#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <stdarg.h>

#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

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

static int os_create_tmpfile_cloexec(char *tmpname)
{
	int fd;

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

	return fd;
}

int os_create_anonymous_file(off_t size)
{
	static const char template[] = "/nemo-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		errno = ENOENT;
		return -1;
	}

	name = malloc(strlen(path) + sizeof(template));
	if (!name)
		return -1;

	strcpy(name, path);
	strcat(name, template);

	fd = os_create_tmpfile_cloexec(name);

	free(name);

	if (fd < 0)
		return -1;

	if (ftruncate(fd, size) < 0) {
		close(fd);
		return -1;
	}

	return fd;
}

int os_exist_path(const char *path)
{
	return access(path, F_OK) == 0;
}

pid_t os_execute_path(const char *path, char *const argv[])
{
	pid_t pid;

	pid = fork();
	if (pid == 0) {
		sigset_t allsigs;

		sigfillset(&allsigs);
		sigprocmask(SIG_UNBLOCK, &allsigs, NULL);

		if (argv == NULL) {
			if (execl(path, path, NULL) < 0)
				exit(-1);
		} else {
			if (execv(path, argv) < 0)
				exit(-1);
		}

		exit(-1);
	}

	return pid;
}

int os_load_path(const char *path, char **buffer, int *size)
{
	FILE *fp;
	char *tbuffer;
	int tsize;

	if (path == NULL)
		return -1;

	fp = fopen(path, "rt");
	if (fp == NULL)
		return -1;

	fseek(fp, 0, SEEK_END);
	tsize = ftell(fp);
	rewind(fp);

	tbuffer = malloc(sizeof(char) * (tsize + 1));
	if (tbuffer == NULL) {
		fclose(fp);
		return -1;
	}
	memset(tbuffer, 0, tsize + 1);

	fread(tbuffer, sizeof(char), tsize, fp);

	fclose(fp);

	if (buffer != NULL)
		*buffer = tbuffer;
	if (size != NULL)
		*size = tsize;

	return 0;
}

int os_save_path(const char *path, char *buffer, int size)
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

int os_append_path(char *path, const char *name)
{
	int len = strlen(path);

	if (name[0] == '/') {
		strcpy(path, name);
	} else if (len <= 0) {
		strcpy(path, "/");
		strcat(path, name);
	} else if (strcmp(name, ".") == 0) {
	} else if (strcmp(name, "..") == 0) {
		int i, state = 0;

		for (i = len - 1; i >= 0; i--) {
			if (path[i] == '/' && state == 0)
				continue;
			if (path[i] == '/' && state == 1)
				break;

			state = 1;
		}

		path[i] = '\0';
	} else {
		if (path[len - 1] != '/')
			strcat(path, "/");
		strcat(path, name);
	}

	return 0;
}

const char *os_get_file_extension(const char *name)
{
	int len = strlen(name);
	int i;

	for (i = len - 1; i >= 0; i--) {
		if (name[i] == '.')
			return &name[i+1];
	}

	return NULL;
}

int os_has_file_extension(const char *name, ...)
{
	const char *ext;
	const char *cmp;
	va_list vargs;

	ext = os_get_file_extension(name);
	if (ext == NULL)
		return 0;

	va_start(vargs, name);

	while ((cmp = va_arg(vargs, const char *)) != NULL) {
		if (strcmp(ext, cmp) == 0)
			return 1;
	}

	va_end(vargs);

	return 0;
}

char *os_get_file_path(const char *name)
{
	char *path = strdup(name);
	int len = strlen(name);
	int i;

	for (i = len - 1; i >= 0; i--) {
		if (path[i] == '/') {
			path[i + 1] = '\0';

			return path;
		}
	}

	free(path);

	return NULL;
}

int os_set_nonblocking_mode(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);

	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
