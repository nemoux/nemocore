#define _GNU_SOURCE
#define __USE_GNU
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

#include <nemobus.h>
#include <nemomisc.h>

struct nemobus *nemobus_create(void)
{
	struct nemobus *bus;

	bus = (struct nemobus *)malloc(sizeof(struct nemobus));
	if (bus == NULL)
		return NULL;
	memset(bus, 0, sizeof(struct nemobus));

	bus->soc = -1;

	return bus;
}

void nemobus_destroy(struct nemobus *bus)
{
	if (bus->soc >= 0)
		close(bus->soc);

	free(bus);
}

int nemobus_connect(struct nemobus *bus, const char *socketpath)
{
	struct sockaddr_un addr;
	socklen_t size, namesize;
	int soc;

	soc = socket(PF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (soc < 0)
		return -1;

	addr.sun_family = AF_LOCAL;
	namesize = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", socketpath != NULL ? socketpath : "/tmp/nemo.bus.0");
	size = offsetof(struct sockaddr_un, sun_path) + namesize;

	if (connect(soc, (struct sockaddr *)&addr, size) < 0)
		goto err1;

	bus->soc = soc;

	return soc;

err1:
	close(soc);

	return -1;
}

void nemobus_disconnect(struct nemobus *bus)
{
	close(bus->soc);

	bus->soc = -1;
}

int nemobus_advertise(struct nemobus *bus, const char *type, const char *path)
{
	return 0;
}

int nemobus_send_raw(struct nemobus *bus, const char *buffer)
{
	return send(bus->soc, buffer, strlen(buffer), MSG_NOSIGNAL | MSG_DONTWAIT);
}

int nemobus_send_format(struct nemobus *bus, const char *fmt, ...)
{
	va_list vargs;
	char *buffer;
	int r;

	va_start(vargs, fmt);
	vasprintf(&buffer, fmt, vargs);
	va_end(vargs);

	r = send(bus->soc, buffer, strlen(buffer), MSG_NOSIGNAL | MSG_DONTWAIT);

	free(buffer);

	return r;
}

int nemobus_recv_raw(struct nemobus *bus, char *buffer, size_t size)
{
	return recv(bus->soc, buffer, size, MSG_NOSIGNAL | MSG_DONTWAIT);
}

int nemobus_unicast(struct nemobus *bus, const char *type, const char *path, struct busmsg *msg)
{
}

int nemobus_multicast(struct nemobus *bus, const char *types, int ntypes, const char *paths, int npaths, struct busmsg *msg)
{
}

int nemobus_broadcast(struct nemobus *bus, struct busmsg *msg)
{
}

struct busmsg *nemobus_msg_create(void)
{
	struct busmsg *msg;

	msg = (struct busmsg *)malloc(sizeof(struct busmsg));
	if (msg == NULL)
		return NULL;
	memset(msg, 0, sizeof(struct busmsg));

	nemolist_init(&msg->attr_list);
	nemolist_init(&msg->msg_list);

	nemolist_init(&msg->link);

	return msg;
}

void nemobus_msg_destroy(struct busmsg *msg)
{
	nemolist_remove(&msg->link);

	nemobus_msg_clear(msg);

	if (msg->name != NULL)
		free(msg->name);

	free(msg);
}

void nemobus_msg_clear(struct busmsg *msg)
{
	struct msgattr *attr, *nattr;
	struct busmsg *cmsg, *nmsg;

	nemolist_for_each_safe(attr, nattr, &msg->attr_list, link) {
		nemolist_remove(&attr->link);

		free(attr->name);
		free(attr->value);

		free(attr);
	}

	nemolist_for_each_safe(cmsg, nmsg, &msg->msg_list, link) {
		nemobus_msg_destroy(cmsg);
	}
}

void nemobus_msg_attach(struct busmsg *msg, struct busmsg *cmsg)
{
	nemolist_insert(&msg->msg_list, &cmsg->link);
}

void nemobus_msg_detach(struct busmsg *msg)
{
	nemolist_remove(&msg->link);
	nemolist_init(&msg->link);
}

void nemobus_msg_set_name(struct busmsg *msg, const char *name)
{
	if (msg->name != NULL)
		free(msg->name);

	msg->name = strdup(name);
}

static struct msgattr *nemobus_msg_get_attr_raw(struct busmsg *msg, const char *name)
{
	struct msgattr *attr;

	nemolist_for_each(attr, &msg->attr_list, link) {
		if (strcmp(attr->name, name) == 0)
			return attr;
	}

	return NULL;
}

void nemobus_msg_set_attr(struct busmsg *msg, const char *name, const char *value)
{
	struct msgattr *attr;

	attr = (struct msgattr *)malloc(sizeof(struct msgattr));
	attr->name = strdup(name);
	attr->value = strdup(value);

	nemolist_insert(&msg->attr_list, &attr->link);
}

const char *nemobus_msg_get_attr(struct busmsg *msg, const char *name)
{
	struct msgattr *attr;

	attr = nemobus_msg_get_attr_raw(msg, name);
	if (attr != NULL)
		return attr->value;

	return NULL;
}

void nemobus_msg_put_attr(struct busmsg *msg, const char *name)
{
	struct msgattr *attr;

	attr = nemobus_msg_get_attr_raw(msg, name);
	if (attr != NULL) {
		nemolist_remove(&attr->link);

		free(attr->name);
		free(attr->value);

		free(attr);
	}
}
