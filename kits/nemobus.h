#ifndef	__NEMO_BUS_H__
#define	__NEMO_BUS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nemolist.h>

struct json_object;

struct nemobus {
	int soc;
};

struct msgattr {
	char *name;
	char *value;

	struct nemolist link;
};

struct busmsg {
	char *name;

	struct nemolist attr_list;
	struct nemolist msg_list;

	struct nemolist link;
};

#define nemobus_attr_for_each(attr, msg)	\
	nemolist_for_each(attr, &((msg)->list), link)
#define nemobus_attr_for_each_safe(attr, tmp, msg)	\
	nemolist_for_each_safe(attr, tmp, &((msg)->list), link)

extern struct nemobus *nemobus_create(void);
extern void nemobus_destroy(struct nemobus *bus);

extern int nemobus_connect(struct nemobus *bus, const char *socketpath);
extern void nemobus_disconnect(struct nemobus *bus);

extern int nemobus_advertise(struct nemobus *bus, const char *type, const char *path);

extern int nemobus_send(struct nemobus *bus, const char *buffer);
extern int nemobus_send_format(struct nemobus *bus, const char *fmt, ...);
extern int nemobus_send_msg(struct nemobus *bus, const char *from, const char *to, struct busmsg *msg);
extern int nemobus_recv(struct nemobus *bus, char *buffer, size_t size);

extern struct busmsg *nemobus_msg_create(void);
extern void nemobus_msg_destroy(struct busmsg *msg);

extern void nemobus_msg_clear(struct busmsg *msg);

extern void nemobus_msg_attach(struct busmsg *msg, struct busmsg *cmsg);
extern void nemobus_msg_detach(struct busmsg *msg);

extern void nemobus_msg_set_name(struct busmsg *msg, const char *name);

extern void nemobus_msg_set_attr(struct busmsg *msg, const char *name, const char *value);
extern void nemobus_msg_set_attr_format(struct busmsg *msg, const char *name, const char *fmt, ...);
extern const char *nemobus_msg_get_attr(struct busmsg *msg, const char *name);
extern void nemobus_msg_put_attr(struct busmsg *msg, const char *name);

extern struct json_object *nemobus_msg_to_json(struct busmsg *msg);
extern const char *nemobus_msg_to_json_string(struct busmsg *msg);
extern struct busmsg *nemobus_msg_from_json(struct json_object *jobj);
extern struct busmsg *nemobus_msg_from_json_string(const char *contents);

static inline int nemobus_get_socket(struct nemobus *bus)
{
	return bus->soc;
}

static inline const char *nemobus_msg_get_name(struct busmsg *msg)
{
	return msg->name;
}

static inline const char *nemobus_attr_get_name(struct msgattr *attr)
{
	return attr->name;
}

static inline const char *nemobus_attr_get_value(struct msgattr *attr)
{
	return attr->value;
}

static inline int nemobus_msg_get_iattr(struct busmsg *msg, const char *name, int value)
{
	const char *str = nemobus_msg_get_attr(msg, name);

	return str != NULL ? strtoul(str, NULL, 10) : value;
}

static inline float nemobus_msg_get_fattr(struct busmsg *msg, const char *name, float value)
{
	const char *str = nemobus_msg_get_attr(msg, name);

	return str != NULL ? strtod(str, NULL) : value;
}

static inline const char *nemobus_msg_get_sattr(struct busmsg *msg, const char *name, const char *value)
{
	const char *str = nemobus_msg_get_attr(msg, name);

	return str != NULL ? str : value;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
