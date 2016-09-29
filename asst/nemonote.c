#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemonote.h>
#include <nemokeys.h>
#include <nemomisc.h>

struct nemonote *nemonote_create(const char *path)
{
	struct nemonote *note;

	note = (struct nemonote *)malloc(sizeof(struct nemonote));
	if (note == NULL)
		return NULL;
	memset(note, 0, sizeof(struct nemonote));

	note->keys = nemokeys_create(path);
	if (note->keys == NULL)
		goto err1;

	return note;

err1:
	free(note);

	return NULL;
}

void nemonote_destroy(struct nemonote *note)
{
	nemokeys_destroy(note->keys);

	free(note);
}

static inline struct json_object *nemonote_get_json_object(struct nemonote *note, const char *ns)
{
	struct json_object *jobj;
	char *contents;

	contents = nemokeys_get(note->keys, ns);
	if (contents != NULL) {
		jobj = json_tokener_parse(contents);

		free(contents);
	} else {
		jobj = json_object_new_object();
	}

	return jobj;
}

int nemonote_set(struct nemonote *note, const char *ns, const char *contents)
{
	nemokeys_set(note->keys, ns, contents);

	return 0;
}

char *nemonote_get(struct nemonote *note, const char *ns)
{
	return nemokeys_get(note->keys, ns);
}

int nemonote_put(struct nemonote *note, const char *ns)
{
	return nemokeys_put(note->keys, ns);
}

int nemonote_set_attr(struct nemonote *note, const char *ns, const char *attr, const char *value)
{
	struct json_object *jobj;

	jobj = nemonote_get_json_object(note, ns);

	json_object_object_add(jobj, attr, json_object_new_string(value));

	nemokeys_set(note->keys, ns, json_object_to_json_string(jobj));

	json_object_put(jobj);

	return 0;
}

char *nemonote_get_attr(struct nemonote *note, const char *ns, const char *attr)
{
	struct json_object *jobj;
	struct json_object *robj;
	char *value = NULL;

	jobj = nemonote_get_json_object(note, ns);

	if (json_object_object_get_ex(jobj, attr, &robj) != 0) {
		value = strdup(json_object_get_string(robj));

		json_object_put(robj);
	}

	json_object_put(jobj);

	return value;
}

void nemonote_put_attr(struct nemonote *note, const char *ns, const char *attr)
{
	struct json_object *jobj;

	if (attr == NULL) {
		nemokeys_put(note->keys, ns);
		return;
	}

	jobj = nemonote_get_json_object(note, ns);

	json_object_object_del(jobj, attr);

	nemokeys_set(note->keys, ns, json_object_to_json_string(jobj));

	json_object_put(jobj);
}

struct noteobject *nemonote_object_ref(struct nemonote *note, const char *ns)
{
	struct noteobject *obj;

	obj = (struct noteobject *)malloc(sizeof(struct noteobject));
	if (obj == NULL)
		return NULL;
	memset(obj, 0, sizeof(struct noteobject));

	obj->ns = strdup(ns);

	obj->jobj = nemonote_get_json_object(note, ns);

	return obj;
}

void nemonote_object_unref(struct nemonote *note, struct noteobject *obj)
{
	if (obj->dirty != 0)
		nemokeys_set(note->keys, obj->ns, json_object_to_json_string(obj->jobj));

	json_object_put(obj->jobj);

	free(obj->ns);
	free(obj);
}

int nemonote_object_set(struct nemonote *note, struct noteobject *obj, const char *attr, const char *value)
{
	json_object_object_add(obj->jobj, attr, json_object_new_string(value));

	obj->dirty = 1;
}

char *nemonote_object_get(struct nemonote *note, struct noteobject *obj, const char *attr)
{
	struct json_object *robj;
	char *value = NULL;

	if (json_object_object_get_ex(obj->jobj, attr, &robj) != 0) {
		value = strdup(json_object_get_string(robj));

		json_object_put(robj);
	}

	return value;
}

void nemonote_object_put(struct nemonote *note, struct noteobject *obj, const char *attr)
{
	json_object_object_del(obj->jobj, attr);

	obj->dirty = 1;
}

struct jsoniter *nemonote_object_create_iterator(struct nemonote *note, struct noteobject *obj)
{
	struct jsoniter *iter;

	iter = (struct jsoniter *)malloc(sizeof(struct jsoniter));
	if (iter == NULL)
		return NULL;
	memset(iter, 0, sizeof(struct jsoniter));

	iter->jobj = obj->jobj;
	iter->needs_free = 0;

	iter->jiter = json_object_iter_begin(iter->jobj);
	iter->jiter0 = json_object_iter_end(iter->jobj);

	if (json_object_iter_equal(&iter->jiter, &iter->jiter0) != 0)
		goto err1;

	return iter;

err1:
	free(iter);

	return NULL;
}

struct noteiter *nemonote_create_iterator(struct nemonote *note)
{
	struct noteiter *iter;

	iter = (struct noteiter *)malloc(sizeof(struct noteiter));
	if (iter == NULL)
		return NULL;

	iter->kiter = nemokeys_create_iterator(note->keys);
	if (iter->kiter == NULL)
		goto err1;
	if (nemokeys_iterator_seek_to_first(iter->kiter) == 0)
		goto err2;

	iter->key = nemokeys_iterator_key_safe(iter->kiter);
	iter->value = nemokeys_iterator_value_safe(iter->kiter);

	return iter;

err2:
	nemokeys_destroy_iterator(iter->kiter);

err1:
	free(iter);

	return NULL;
}

void nemonote_destroy_iterator(struct noteiter *iter)
{
	nemokeys_destroy_iterator(iter->kiter);

	if (iter->key != NULL)
		free(iter->key);
	if (iter->value != NULL)
		free(iter->value);

	free(iter);
}

int nemonote_iterator_next(struct noteiter *iter)
{
	if (iter->key != NULL)
		free(iter->key);
	if (iter->value != NULL)
		free(iter->value);

	if (nemokeys_iterator_next(iter->kiter) == 0) {
		iter->key = NULL;
		iter->value = NULL;

		return 0;
	}

	iter->key = nemokeys_iterator_key_safe(iter->kiter);
	iter->value = nemokeys_iterator_value_safe(iter->kiter);

	return 1;
}

const char *nemonote_iterator_key(struct noteiter *iter)
{
	return iter->key;
}

const char *nemonote_iterator_value(struct noteiter *iter)
{
	return iter->value;
}

struct noteobject *nemonote_iterator_object(struct noteiter *iter)
{
	struct noteobject *obj;
	char *key;
	char *value;

	obj = (struct noteobject *)malloc(sizeof(struct noteobject));
	if (obj == NULL)
		return NULL;
	memset(obj, 0, sizeof(struct noteobject));

	key = nemokeys_iterator_key_safe(iter->kiter);
	value = nemokeys_iterator_value_safe(iter->kiter);

	obj->ns = key;
	obj->jobj = json_tokener_parse(value);

	free(value);

	return obj;
}

struct jsoniter *nemonote_create_json_iterator(const char *contents)
{
	struct jsoniter *iter;

	iter = (struct jsoniter *)malloc(sizeof(struct jsoniter));
	if (iter == NULL)
		return NULL;
	memset(iter, 0, sizeof(struct jsoniter));

	iter->jobj = json_tokener_parse(contents);
	iter->needs_free = 1;

	iter->jiter = json_object_iter_begin(iter->jobj);
	iter->jiter0 = json_object_iter_end(iter->jobj);

	if (json_object_iter_equal(&iter->jiter, &iter->jiter0) != 0)
		goto err1;

	return iter;

err1:
	json_object_put(iter->jobj);

	free(iter);

	return NULL;
}

void nemonote_destroy_json_iterator(struct jsoniter *iter)
{
	if (iter->needs_free != 0)
		json_object_put(iter->jobj);

	free(iter);
}

int nemonote_json_iterator_next(struct jsoniter *iter)
{
	json_object_iter_next(&iter->jiter);

	return json_object_iter_equal(&iter->jiter, &iter->jiter0) == 0;
}

const char *nemonote_json_iterator_key(struct jsoniter *iter)
{
	return json_object_iter_peek_name(&iter->jiter);
}

const char *nemonote_json_iterator_value(struct jsoniter *iter)
{
	return json_object_get_string(json_object_iter_peek_value(&iter->jiter));
}
