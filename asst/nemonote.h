#ifndef	__NEMO_NOTE_H__
#define	__NEMO_NOTE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <json.h>

#include <nemokeys.h>

struct nemonote {
	struct nemokeys *keys;
};

struct noteobject {
	struct json_object *jobj;

	char *ns;
	int dirty;
};

struct noteiter {
	struct keysiter *kiter;

	char *key;
	char *value;
};

struct jsoniter {
	struct json_object *jobj;
	int needs_free;

	struct json_object_iterator jiter;
	struct json_object_iterator jiter0;
};

extern struct nemonote *nemonote_create(const char *path);
extern void nemonote_destroy(struct nemonote *note);

extern void nemonote_clear(struct nemonote *note);

extern int nemonote_set(struct nemonote *note, const char *ns, const char *contents);
extern char *nemonote_get(struct nemonote *note, const char *ns);
extern int nemonote_put(struct nemonote *note, const char *ns);

extern int nemonote_set_attr(struct nemonote *note, const char *ns, const char *attr, const char *value);
extern char *nemonote_get_attr(struct nemonote *note, const char *ns, const char *attr);
extern void nemonote_put_attr(struct nemonote *note, const char *ns, const char *attr);

extern struct noteobject *nemonote_object_ref(struct nemonote *note, const char *ns);
extern void nemonote_object_unref(struct nemonote *note, struct noteobject *obj);
extern int nemonote_object_set(struct nemonote *note, struct noteobject *obj, const char *attr, const char *value);
extern char *nemonote_object_get(struct nemonote *note, struct noteobject *obj, const char *attr);
extern void nemonote_object_put(struct nemonote *note, struct noteobject *obj, const char *attr);
extern struct jsoniter *nemonote_object_create_iterator(struct nemonote *note, struct noteobject *obj);

extern struct noteiter *nemonote_create_iterator(struct nemonote *note);
extern void nemonote_destroy_iterator(struct noteiter *iter);

extern int nemonote_iterator_next(struct noteiter *iter);
extern const char *nemonote_iterator_key(struct noteiter *iter);
extern const char *nemonote_iterator_value(struct noteiter *iter);
extern struct noteobject *nemonote_iterator_object(struct noteiter *iter);

extern struct jsoniter *nemonote_json_iterator_create(const char *contents);
extern void nemonote_json_iterator_destroy(struct jsoniter *iter);
extern int nemonote_json_iterator_next(struct jsoniter *iter);
extern const char *nemonote_json_iterator_key(struct jsoniter *iter);
extern const char *nemonote_json_iterator_value(struct jsoniter *iter);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
