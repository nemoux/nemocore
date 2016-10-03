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
	return nemobus_send_format(bus, "{ \"to\": \"/nemobusd\", \"advertise\": { \"type\": \"%s\", \"path\": \"%s\" } }", type, path);
}

int nemobus_send(struct nemobus *bus, const char *from, const char *to, struct busmsg *msg)
{
	struct json_object *jobj;
	const char *contents;
	int r;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "from", json_object_new_string(from));
	json_object_object_add(jobj, "to", json_object_new_string(to));
	json_object_object_add(jobj, nemobus_msg_get_name(msg), nemobus_msg_to_json(msg));

	contents = json_object_get_string(jobj);

	r = send(bus->soc, contents, strlen(contents), MSG_NOSIGNAL | MSG_DONTWAIT);

	json_object_put(jobj);

	return r;
}

int nemobus_send_many(struct nemobus *bus, const char *from, const char *to, struct busmsg **msgs, int count)
{
	struct json_object *jobj;
	const char *contents;
	int r;
	int i;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "from", json_object_new_string(from));
	json_object_object_add(jobj, "to", json_object_new_string(to));

	for (i = 0; i < count; i++)
		json_object_object_add(jobj, nemobus_msg_get_name(msgs[i]), nemobus_msg_to_json(msgs[i]));

	contents = json_object_get_string(jobj);

	r = send(bus->soc, contents, strlen(contents), MSG_NOSIGNAL | MSG_DONTWAIT);

	json_object_put(jobj);

	return r;
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

struct busmsg *nemobus_recv(struct nemobus *bus)
{
	struct busmsg *msg;
	char buffer[4096];
	int r;

	r = recv(bus->soc, buffer, sizeof(buffer), MSG_NOSIGNAL | MSG_DONTWAIT);
	if (r <= 0)
		return NULL;
	buffer[r] = '\0';

	return nemobus_msg_from_json_string(buffer);
}

struct nemoitem *nemobus_recv_item(struct nemobus *bus)
{
	struct nemoitem *item;
	struct json_object *jobj;
	char buffer[4096];
	int r;

	r = recv(bus->soc, buffer, sizeof(buffer), MSG_NOSIGNAL | MSG_DONTWAIT);
	if (r <= 0)
		return NULL;
	buffer[r] = '\0';

	jobj = json_tokener_parse(buffer);
	if (jobj != NULL) {
		struct json_object *pobj;

		if (json_object_object_get_ex(jobj, "to", &pobj) != 0) {
			item = nemoitem_create();

			nemoitem_load_json(item, json_object_get_string(pobj), jobj);

			json_object_put(pobj);
		}

		json_object_put(jobj);
	}

	return item;
}

int nemobus_recv_raw(struct nemobus *bus, char *buffer, size_t size)
{
	return recv(bus->soc, buffer, size, MSG_NOSIGNAL | MSG_DONTWAIT);
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

	if (msg->elems != NULL)
		free(msg->elems);

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

	if (msg->elems != NULL) {
		int i;

		for (i = 0; msg->nelems; i++) {
			if (msg->elems[i] != NULL) {
				free(msg->elems[i]);

				msg->elems[i] = NULL;
			}
		}

		msg->nelems = 0;
	}

	nemolist_for_each_safe(cmsg, nmsg, &msg->msg_list, link) {
		nemobus_msg_destroy(cmsg);
	}
}

void nemobus_msg_attach(struct busmsg *msg, struct busmsg *cmsg)
{
	nemolist_insert_tail(&msg->msg_list, &cmsg->link);
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

static inline struct msgattr *nemobus_msg_get_attr_in(struct busmsg *msg, const char *name)
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

void nemobus_msg_set_attr_format(struct busmsg *msg, const char *name, const char *fmt, ...)
{
	struct msgattr *attr;
	va_list vargs;
	char *value;

	va_start(vargs, fmt);
	vasprintf(&value, fmt, vargs);
	va_end(vargs);

	attr = (struct msgattr *)malloc(sizeof(struct msgattr));
	attr->name = strdup(name);
	attr->value = value;

	nemolist_insert(&msg->attr_list, &attr->link);
}

const char *nemobus_msg_get_attr(struct busmsg *msg, const char *name)
{
	struct msgattr *attr;

	attr = nemobus_msg_get_attr_in(msg, name);
	if (attr != NULL)
		return attr->value;

	return NULL;
}

void nemobus_msg_put_attr(struct busmsg *msg, const char *name)
{
	struct msgattr *attr;

	attr = nemobus_msg_get_attr_in(msg, name);
	if (attr != NULL) {
		nemolist_remove(&attr->link);

		free(attr->name);
		free(attr->value);

		free(attr);
	}
}

static inline void nemobus_msg_set_elem_in(struct busmsg *msg, int index)
{
	if (msg->selems <= index) {
		char **elems;
		int selems = MAX(index * 2, 8);

		elems = (char **)malloc(sizeof(char *) * selems);
		memset(elems, 0, sizeof(char *) * selems);
		memcpy(elems, msg->elems, sizeof(char *) * msg->selems);
		free(msg->elems);

		msg->elems = elems;
		msg->selems = selems;
	}
}

void nemobus_msg_set_elem(struct busmsg *msg, int index, const char *value)
{
	nemobus_msg_set_elem_in(msg, index);

	if (msg->elems[index] != NULL)
		free(msg->elems[index]);

	msg->elems[index] = strdup(value);

	msg->nelems = MAX(msg->nelems, index);
}

void nemobus_msg_set_elem_format(struct busmsg *msg, int index, const char *fmt, ...)
{
	va_list vargs;
	char *value;

	va_start(vargs, fmt);
	vasprintf(&value, fmt, vargs);
	va_end(vargs);

	nemobus_msg_set_elem_in(msg, index);

	if (msg->elems[index] != NULL)
		free(msg->elems[index]);

	msg->elems[index] = value;

	msg->nelems = MAX(msg->nelems, index);
}

const char *nemobus_msg_get_elem(struct busmsg *msg, int index)
{
	return index < msg->selems ? msg->elems[index] : NULL;
}

void nemobus_msg_put_elem(struct busmsg *msg, int index)
{
	if (index >= msg->selems)
		return;

	if (msg->elems[index] != NULL) {
		free(msg->elems[index]);

		msg->elems[index] = NULL;
	}
}

int nemobus_msg_get_elements_length(struct busmsg *msg)
{
	return msg->nelems;
}

struct json_object *nemobus_msg_to_json(struct busmsg *msg)
{
	struct json_object *jobj;
	struct msgattr *attr;
	struct busmsg *cmsg;

	if (msg->elems == NULL) {
		jobj = json_object_new_object();

		nemolist_for_each(attr, &msg->attr_list, link) {
			json_object_object_add(jobj, attr->name, json_object_new_string(attr->value));
		}

		nemolist_for_each(cmsg, &msg->msg_list, link) {
			json_object_object_add(jobj, cmsg->name, nemobus_msg_to_json(cmsg));
		}
	} else {
		int i;

		jobj = json_object_new_array();

		for (i = 0; i < msg->nelems; i++) {
			if (msg->elems[i] != NULL)
				json_object_array_add(jobj, json_object_new_string(msg->elems[i]));
		}
	}

	return jobj;
}

const char *nemobus_msg_to_json_string(struct busmsg *msg)
{
	return json_object_get_string(nemobus_msg_to_json(msg));
}

struct busmsg *nemobus_msg_from_json(struct json_object *jobj)
{
	struct busmsg *msg;
	struct busmsg *cmsg;

	msg = nemobus_msg_create();

	if (json_object_is_type(jobj, json_type_object)) {
		json_object_object_foreach(jobj, key, value) {
			if (json_object_is_type(value, json_type_string)) {
				nemobus_msg_set_attr(msg, key, json_object_get_string(value));
			} else if (json_object_is_type(value, json_type_object) || json_object_is_type(value, json_type_array)) {
				cmsg = nemobus_msg_from_json(value);
				nemobus_msg_set_name(cmsg, key);

				nemobus_msg_attach(msg, cmsg);
			}
		}
	} else if (json_object_is_type(jobj, json_type_array)) {
		struct json_object *cobj;
		int i;

		for (i = 0; i < json_object_array_length(jobj); i++) {
			cobj = json_object_array_get_idx(jobj, i);

			nemobus_msg_set_elem(msg, i, json_object_get_string(cobj));
		}
	}

	return msg;
}

struct busmsg *nemobus_msg_from_json_string(const char *contents)
{
	struct json_object *jobj;
	struct busmsg *msg = NULL;

	jobj = json_tokener_parse(contents);
	if (jobj != NULL) {
		msg = nemobus_msg_from_json(jobj);

		json_object_put(jobj);
	}

	return msg;
}
