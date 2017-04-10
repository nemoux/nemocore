#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/socket.h>
#include <wayland-server.h>

#include <waylandhelper.h>
#include <nemomisc.h>
#include <nemolog.h>

static inline void wayland_execute_client_in(int sockfd, const char *path, char *const argv[], char *const envp[])
{
	int clientfd;
	char fdenv[32];
	sigset_t allsigs;

	sigfillset(&allsigs);
	sigprocmask(SIG_UNBLOCK, &allsigs, NULL);

	if (seteuid(getuid()) == -1)
		return;

	clientfd = dup(sockfd);
	if (clientfd == -1) {
		return;
	}

	if (argv == NULL || argv[0] == NULL) {
		env_set_integer("WAYLAND_SOCKET", clientfd);

		if (execl(path, path, NULL) < 0)
			nemolog_warning("WAYLAND", "failed to execute '%s' with errno %d\n", path, errno);
	} else if (envp == NULL || envp[0] == NULL) {
		env_set_integer("WAYLAND_SOCKET", clientfd);

		if (execv(path, argv) < 0)
			nemolog_warning("WAYLAND", "failed to execute '%s' with errno %d\n", path, errno);
	} else {
		char *_envp[16];
		int i;

		snprintf(fdenv, sizeof(fdenv), "WAYLAND_SOCKET=%d", clientfd);

		for (i = 0; envp[i] != NULL; i++)
			_envp[i] = envp[i];
		_envp[i++] = fdenv;
		_envp[i++] = NULL;

		if (execve(path, argv, _envp) < 0)
			nemolog_warning("WAYLAND", "failed to execute '%s' with errno %d\n", path, errno);
	}
}

struct wl_client *wayland_execute_client(struct wl_display *display, const char *path, char *const argv[], char *const envp[])
{
	int sv[2];
	pid_t pid;
	struct wl_client *client;

	if (os_socketpair_cloexec(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
		return NULL;
	}

	pid = fork();
	if (pid == -1) {
		close(sv[0]);
		close(sv[1]);
		return NULL;
	}

	if (pid == 0) {
		wayland_execute_client_in(sv[1], path, argv, envp);

		exit(EXIT_FAILURE);
	}

	close(sv[1]);

	client = wl_client_create(display, sv[0]);
	if (client == NULL) {
		close(sv[0]);
		return NULL;
	}

	return client;
}

void wayland_transform_point(int width, int height, enum wl_output_transform transform, int32_t scale, float sx, float sy, float *bx, float *by)
{
	switch (transform) {
		case WL_OUTPUT_TRANSFORM_NORMAL:
		default:
			*bx = sx;
			*by = sy;
			break;
		case WL_OUTPUT_TRANSFORM_FLIPPED:
			*bx = width - sx;
			*by = sy;
			break;
		case WL_OUTPUT_TRANSFORM_90:
			*bx = height - sy;
			*by = sx;
			break;
		case WL_OUTPUT_TRANSFORM_FLIPPED_90:
			*bx = height - sy;
			*by = width - sx;
			break;
		case WL_OUTPUT_TRANSFORM_180:
			*bx = width - sx;
			*by = height - sy;
			break;
		case WL_OUTPUT_TRANSFORM_FLIPPED_180:
			*bx = sx;
			*by = height - sy;
			break;
		case WL_OUTPUT_TRANSFORM_270:
			*bx = sy;
			*by = width - sx;
			break;
		case WL_OUTPUT_TRANSFORM_FLIPPED_270:
			*bx = sy;
			*by = sx;
			break;
	}

	*bx *= scale;
	*by *= scale;
}

pixman_box32_t wayland_transform_rect(int width, int height, enum wl_output_transform transform, int32_t scale, pixman_box32_t rect)
{
	float x1, x2, y1, y2;
	pixman_box32_t box;

	wayland_transform_point(width, height, transform, scale, rect.x1, rect.y1, &x1, &y1);
	wayland_transform_point(width, height, transform, scale, rect.x2, rect.y2, &x2, &y2);

	if (x1 <= x2) {
		box.x1 = x1;
		box.x2 = x2;
	} else {
		box.x1 = x2;
		box.x2 = x1;
	}

	if (y1 <= y2) {
		box.y1 = y1;
		box.y2 = y2;
	} else {
		box.y1 = y2;
		box.y2 = y1;
	}

	return box;
}

void wayland_transform_region(int width, int height, enum wl_output_transform transform, int32_t scale, pixman_region32_t *src, pixman_region32_t *dest)
{
	pixman_box32_t *sboxes, *dboxes;
	int nboxes, i;

	if (transform == WL_OUTPUT_TRANSFORM_NORMAL && scale == 1) {
		if (src != dest)
			pixman_region32_copy(dest, src);
		return;
	}

	sboxes = pixman_region32_rectangles(src, &nboxes);
	dboxes = (pixman_box32_t *)malloc(nboxes * sizeof(pixman_box32_t));
	if (!dboxes)
		return;

	if (transform == WL_OUTPUT_TRANSFORM_NORMAL) {
		memcpy(dboxes, sboxes, nboxes * sizeof(pixman_box32_t));
	} else {
		for (i = 0; i < nboxes; i++) {
			switch (transform) {
				default:
				case WL_OUTPUT_TRANSFORM_NORMAL:
					dboxes[i].x1 = sboxes[i].x1;
					dboxes[i].y1 = sboxes[i].y1;
					dboxes[i].x2 = sboxes[i].x2;
					dboxes[i].y2 = sboxes[i].y2;
					break;
				case WL_OUTPUT_TRANSFORM_90:
					dboxes[i].x1 = height - sboxes[i].y2;
					dboxes[i].y1 = sboxes[i].x1;
					dboxes[i].x2 = height - sboxes[i].y1;
					dboxes[i].y2 = sboxes[i].x2;
					break;
				case WL_OUTPUT_TRANSFORM_180:
					dboxes[i].x1 = width - sboxes[i].x2;
					dboxes[i].y1 = height - sboxes[i].y2;
					dboxes[i].x2 = width - sboxes[i].x1;
					dboxes[i].y2 = height - sboxes[i].y1;
					break;
				case WL_OUTPUT_TRANSFORM_270:
					dboxes[i].x1 = sboxes[i].y1;
					dboxes[i].y1 = width - sboxes[i].x2;
					dboxes[i].x2 = sboxes[i].y2;
					dboxes[i].y2 = width - sboxes[i].x1;
					break;
				case WL_OUTPUT_TRANSFORM_FLIPPED:
					dboxes[i].x1 = width - sboxes[i].x2;
					dboxes[i].y1 = sboxes[i].y1;
					dboxes[i].x2 = width - sboxes[i].x1;
					dboxes[i].y2 = sboxes[i].y2;
					break;
				case WL_OUTPUT_TRANSFORM_FLIPPED_90:
					dboxes[i].x1 = height - sboxes[i].y2;
					dboxes[i].y1 = width - sboxes[i].x2;
					dboxes[i].x2 = height - sboxes[i].y1;
					dboxes[i].y2 = width - sboxes[i].x1;
					break;
				case WL_OUTPUT_TRANSFORM_FLIPPED_180:
					dboxes[i].x1 = sboxes[i].x1;
					dboxes[i].y1 = height - sboxes[i].y2;
					dboxes[i].x2 = sboxes[i].x2;
					dboxes[i].y2 = height - sboxes[i].y1;
					break;
				case WL_OUTPUT_TRANSFORM_FLIPPED_270:
					dboxes[i].x1 = sboxes[i].y1;
					dboxes[i].y1 = sboxes[i].x1;
					dboxes[i].x2 = sboxes[i].y2;
					dboxes[i].y2 = sboxes[i].x2;
					break;
			}
		}
	}

	if (scale != 1) {
		for (i = 0; i < nboxes; i++) {
			dboxes[i].x1 *= scale;
			dboxes[i].x2 *= scale;
			dboxes[i].y1 *= scale;
			dboxes[i].y2 *= scale;
		}
	}

	pixman_region32_clear(dest);
	pixman_region32_init_rects(dest, dboxes, nboxes);
	free(dboxes);
}
