#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>

static int set_cloexec_or_close(int fd)
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

	sv[0] = set_cloexec_or_close(sv[0]);
	sv[1] = set_cloexec_or_close(sv[1]);

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
	return set_cloexec_or_close(fd);
}

static int create_tmpfile_cloexec(char *tmpname)
{
	int fd;

#ifdef HAVE_MKOSTEMP
	fd = mkostemp(tmpname, O_CLOEXEC);
	if (fd >= 0)
		unlink(tmpname);
#else
	fd = mkstemp(tmpname);
	if (fd >= 0) {
		fd = set_cloexec_or_close(fd);
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

	fd = create_tmpfile_cloexec(name);

	free(name);

	if (fd < 0)
		return -1;

	if (ftruncate(fd, size) < 0) {
		close(fd);
		return -1;
	}

	return fd;
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
	FILE *file;
	char *tbuffer;
	int tsize;

	if (path == NULL)
		return -1;

	file = fopen(path, "rt");
	if (file == NULL)
		return -1;

	fseek(file, 0, SEEK_END);
	tsize = ftell(file);
	rewind(file);

	tbuffer = malloc(sizeof(char) * (tsize + 1));
	if (tbuffer == NULL) {
		fclose(file);
		return -1;
	}

	fread(tbuffer, sizeof(char), tsize, file);

	fclose(file);

	if (buffer != NULL)
		*buffer = tbuffer;
	if (size != NULL)
		*size = tsize;

	return 0;
}

int os_save_path(const char *path, char *buffer, int size)
{
	FILE *file;

	if (path == NULL)
		return -1;

	file = fopen(path, "w");
	if (file == NULL)
		return -1;

	fwrite(buffer, sizeof(char), size, file);

	fclose(file);

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
