#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <json.h>

#include <nemojson.h>
#include <nemomisc.h>

struct nemojson *nemojson_create_string(const char *str, int length)
{
	struct nemojson *json;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	json->contents = (char *)malloc(length + 1);
	if (json->contents == NULL)
		goto err1;
	memcpy(json->contents, str, length);

	json->length = length;

	return json;

err1:
	free(json);

	return NULL;
}

struct nemojson *nemojson_create_format(const char *fmt, ...)
{
	struct nemojson *json;
	va_list vargs;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	va_start(vargs, fmt);
	vasprintf(&json->contents, fmt, vargs);
	va_end(vargs);

	json->length = strlen(json->contents);

	return json;
}

struct nemojson *nemojson_create_file(const char *filepath)
{
	struct nemojson *json;
	char *buffer = NULL;
	int length;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	length = os_file_load(filepath, &buffer);
	if (length <= 0)
		goto err1;

	json->contents = buffer;
	json->length = length;

	return json;

err1:
	free(json);

	return NULL;
}

struct nemojson *nemojson_create(void)
{
	struct nemojson *json;

	json = (struct nemojson *)malloc(sizeof(struct nemojson));
	if (json == NULL)
		return NULL;
	memset(json, 0, sizeof(struct nemojson));

	json->contents = (char *)malloc(1);
	json->contents[0] = '\0';
	json->length = 0;

	return json;
}

void nemojson_destroy(struct nemojson *json)
{
	int i;

	for (i = 0; i < json->count; i++) {
		if (json->jobjs[i] != NULL)
			json_object_put(json->jobjs[i]);
	}

	for (i = 0; i < json->count; i++) {
		if (json->jkeys[i] != NULL)
			free(json->jkeys[i]);
	}

	free(json->contents);
	free(json);
}

int nemojson_append(struct nemojson *json, const char *str, int length)
{
	char *contents;

	contents = (char *)malloc(json->length + length + 1);
	if (contents == NULL)
		return -1;

	strcpy(contents, json->contents);
	strcat(contents, str);
	contents[json->length + length] = '\0';

	free(json->contents);

	json->contents = contents;
	json->length = json->length + length;

	return 0;
}

int nemojson_append_one(struct nemojson *json, char c)
{
	char *contents;

	contents = (char *)malloc(json->length + 1 + 1);
	if (contents == NULL)
		return -1;

	snprintf(contents, json->length + 1 + 1, "%s%c", json->contents, c);

	free(json->contents);

	json->contents = contents;
	json->length = json->length + 1;

	return 0;
}

int nemojson_append_format(struct nemojson *json, const char *fmt, ...)
{
	va_list vargs;
	char *contents;
	char *str;
	int length;

	va_start(vargs, fmt);
	vasprintf(&str, fmt, vargs);
	va_end(vargs);

	length = strlen(str);

	contents = (char *)malloc(json->length + length + 1);
	if (contents == NULL)
		return -1;

	strcpy(contents, json->contents);
	strcat(contents, str);
	contents[json->length + length] = '\0';

	free(json->contents);
	free(str);

	json->contents = contents;
	json->length = json->length + length;

	return 0;
}

int nemojson_append_file(struct nemojson *json, const char *filepath)
{
	char *contents;
	char *buffer;
	int length;

	length = os_file_load(filepath, &buffer);
	if (length <= 0)
		return -1;

	contents = (char *)malloc(json->length + length + 1);
	if (contents == NULL)
		goto err1;

	strcpy(contents, json->contents);
	strcat(contents, buffer);
	contents[json->length + length] = '\0';

	free(json->contents);
	free(buffer);

	json->contents = contents;
	json->length = json->length + length;

	return 0;

err1:
	free(buffer);

	return -1;
}

int nemojson_update(struct nemojson *json)
{
	struct json_tokener *jtok;
	struct json_object *jobj;
	char *msg;
	int count = 0;

	jtok = json_tokener_new();

	for (msg = json->contents; msg < json->contents + json->length; msg += jtok->char_offset) {
		jobj = json_tokener_parse_ex(jtok, msg, strlen(msg));
		if (jobj == NULL)
			break;

		json->jobjs[json->count++] = jobj;

		count++;
	}

	json_tokener_free(jtok);

	return count;
}

int nemojson_insert_object(struct nemojson *json, const char *jkey, struct json_object *jobj)
{
	if (jkey != NULL)
		json->jkeys[json->count] = strdup(jkey);

	json->jobjs[json->count] = json_object_get(jobj);

	json->count++;

	return 0;
}

int nemojson_iterate_object(struct nemojson *json, struct json_object *jobj)
{
	if (json_object_is_type(jobj, json_type_object)) {
		struct json_object_iterator citer = json_object_iter_begin(jobj);
		struct json_object_iterator eiter = json_object_iter_end(jobj);
		int count = 0;

		while (json_object_iter_equal(&citer, &eiter) == 0) {
			const char *ikey = json_object_iter_peek_name(&citer);
			struct json_object *iobj = json_object_iter_peek_value(&citer);

			json->jobjs[json->count] = json_object_get(iobj);
			json->jkeys[json->count] = strdup(ikey);
			json->count++;

			count++;

			json_object_iter_next(&citer);
		}

		return count;
	} else if (json_object_is_type(jobj, json_type_array)) {
		int i;

		for (i = 0; i < json_object_array_length(jobj); i++) {
			struct json_object *cobj = json_object_array_get_idx(jobj, i);

			json->jobjs[json->count] = json_object_get(cobj);
			json->count++;
		}

		return json_object_array_length(jobj);
	}

	return 0;
}

static inline struct json_object *nemojson_search_object_vargs(struct nemojson *json, int index, int depth, va_list vargs)
{
	struct json_object *jobj;
	const char *key;
	int i;

	jobj = json->jobjs[index];
	if (jobj == NULL)
		return NULL;

	for (i = 0; i < depth; i++) {
		key = va_arg(vargs, const char *);
		if ('0' <= key[0] && key[0] <= '9') {
			if (json_object_is_type(jobj, json_type_array) == 0)
				goto nofound;
			if ((jobj = json_object_array_get_idx(jobj, strtoul(key, NULL, 10))) == NULL)
				goto nofound;
		} else {
			if (json_object_is_type(jobj, json_type_object) == 0)
				goto nofound;
			if (json_object_object_get_ex(jobj, key, &jobj) == 0)
				goto nofound;
		}
	}

	return jobj;

nofound:
	return NULL;
}

struct json_object *nemojson_search_object(struct nemojson *json, int index, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	return jobj;
}

int nemojson_search_integer(struct nemojson *json, int index, int value, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	if (jobj == NULL)
		return value;

	return json_object_get_int(jobj);
}

double nemojson_search_double(struct nemojson *json, int index, double value, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	if (jobj == NULL)
		return value;

	return json_object_get_double(jobj);
}

const char *nemojson_search_string(struct nemojson *json, int index, const char *value, int depth, ...)
{
	struct json_object *jobj;
	va_list vargs;

	va_start(vargs, depth);

	jobj = nemojson_search_object_vargs(json, index, depth, vargs);

	va_end(vargs);

	if (jobj == NULL)
		return value;

	return json_object_get_string(jobj);
}

struct json_object *nemojson_search_attribute(struct json_object *jobj, const char *key, const char *value)
{
	if (json_object_is_type(jobj, json_type_object) != 0) {
		struct json_object_iterator citer = json_object_iter_begin(jobj);
		struct json_object_iterator eiter = json_object_iter_end(jobj);
		struct json_object *tobj;

		while (json_object_iter_equal(&citer, &eiter) == 0) {
			const char *ikey = json_object_iter_peek_name(&citer);
			struct json_object *iobj = json_object_iter_peek_value(&citer);

			if (json_object_is_type(iobj, json_type_string) != 0 &&
					strcmp(ikey, key) == 0 &&
					strcmp(json_object_get_string(iobj), value) == 0)
				return jobj;

			tobj = nemojson_search_attribute(iobj, key, value);
			if (tobj != NULL)
				return tobj;

			json_object_iter_next(&citer);
		}
	} else if (json_object_is_type(jobj, json_type_array) != 0) {
		struct json_object *tobj;
		int i;

		for (i = 0; i < json_object_array_length(jobj); i++) {
			tobj = nemojson_search_attribute(json_object_array_get_idx(jobj, i), key, value);
			if (tobj != NULL)
				return tobj;
		}
	}

	return NULL;
}

struct json_object *nemojson_get_object(struct nemojson *json, int index)
{
	return json->jobjs[index];
}

const char *nemojson_get_string(struct nemojson *json, int index)
{
	return json_object_get_string(json->jobjs[index]);
}

double nemojson_get_double(struct nemojson *json, int index)
{
	return json_object_get_double(json->jobjs[index]);
}

int nemojson_get_integer(struct nemojson *json, int index)
{
	return json_object_get_int(json->jobjs[index]);
}

int nemojson_get_boolean(struct nemojson *json, int index)
{
	return json_object_get_boolean(json->jobjs[index]);
}

struct json_object *nemojson_object_create_file(const char *filepath)
{
	struct json_object *jobj;
	char *buffer;
	int length;

	length = os_file_load(filepath, &buffer);
	if (length <= 0)
		return NULL;

	jobj = json_tokener_parse(buffer);

	free(buffer);

	return jobj;
}

struct json_object *nemojson_object_get_object(struct json_object *jobj, const char *name, struct json_object *value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return tobj;
}

const char *nemojson_object_get_string(struct json_object *jobj, const char *name, const char *value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_string(tobj);
}

char *nemojson_object_dup_string(struct json_object *jobj, const char *name, const char *value)
{
	struct json_object *tobj;
	const char *tstr;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value != NULL ? strdup(value) : NULL;

	tstr = json_object_get_string(tobj);

	return tstr != NULL ? strdup(tstr) : NULL;
}

double nemojson_object_get_double(struct json_object *jobj, const char *name, double value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_double(tobj);
}

int nemojson_object_get_integer(struct json_object *jobj, const char *name, int value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_int(tobj);
}

int nemojson_object_get_boolean(struct json_object *jobj, const char *name, int value)
{
	struct json_object *tobj;

	if (json_object_object_get_ex(jobj, name, &tobj) == 0)
		return value;

	return json_object_get_boolean(tobj) == TRUE;
}

int nemojson_array_get_length(struct json_object *jobj)
{
	return json_object_array_length(jobj);
}

struct json_object *nemojson_array_get_object(struct json_object *jobj, int index)
{
	return json_object_array_get_idx(jobj, index);
}

const char *nemojson_array_get_string(struct json_object *jobj, int index)
{
	return json_object_get_string(json_object_array_get_idx(jobj, index));
}

double nemojson_array_get_double(struct json_object *jobj, int index)
{
	return json_object_get_double(json_object_array_get_idx(jobj, index));
}

int nemojson_array_get_integer(struct json_object *jobj, int index)
{
	return json_object_get_int(json_object_array_get_idx(jobj, index));
}

int nemojson_array_get_boolean(struct json_object *jobj, int index)
{
	return json_object_get_boolean(json_object_array_get_idx(jobj, index)) == TRUE;
}

static int nemojson_object_load_item_inherit(struct json_object *jobj, struct nemoitem *item, struct itemone *one)
{
	struct itemone *cone;

	json_object_object_foreach(jobj, key, value) {
		if (json_object_is_type(value, json_type_object)) {
			cone = nemoitem_one_create();
			nemoitem_one_set_path_format(cone, "%s/%s", one->path, key);
			nemoitem_attach_one(item, cone);

			nemojson_object_load_item_inherit(value, item, cone);
		} else if (json_object_is_type(value, json_type_string)) {
			nemoitem_one_set_attr(one, key, json_object_get_string(value));
		}
	}

	return 0;
}

int nemojson_object_load_item(struct json_object *jobj, struct nemoitem *item, const char *prefix)
{
	struct itemone *one;

	one = nemoitem_one_create();
	nemoitem_one_set_path(one, prefix);
	nemoitem_attach_one(item, one);

	nemojson_object_load_item_inherit(jobj, item, one);

	return 0;
}

int nemojson_string_load_item(const char *contents, struct nemoitem *item, const char *prefix)
{
	struct json_object *jobj;
	int r = 0;

	jobj = json_tokener_parse(contents);
	if (jobj != NULL) {
		r = nemojson_object_load_item(jobj, item, prefix);

		json_object_put(jobj);
	}

	return r;
}

int nemojson_object_load_item_one(struct json_object *jobj, struct itemone *one)
{
	json_object_object_foreach(jobj, key, value) {
		nemoitem_one_set_attr(one, key, json_object_get_string(value));
	}

	return 0;
}

int nemojson_string_load_item_one(const char *contents, struct itemone *one)
{
	struct json_object *jobj;
	int r = 0;

	jobj = json_tokener_parse(contents);
	if (jobj != NULL) {
		r = nemojson_object_load_item_one(jobj, one);

		json_object_put(jobj);
	}

	return r;
}
