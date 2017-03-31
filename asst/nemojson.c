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

struct nemojson *nemojson_create(const char *str, int length)
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

void nemojson_destroy(struct nemojson *json)
{
	int i;

	for (i = 0; i < json->count; i++)
		json_object_put(json->jobjs[i]);

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

void nemojson_update(struct nemojson *json)
{
	struct json_tokener *jtok;
	struct json_object *jobj;
	char *msg;

	jtok = json_tokener_new();

	for (msg = json->contents; msg < json->contents + json->length; msg += jtok->char_offset) {
		jobj = json_tokener_parse_ex(jtok, msg, strlen(msg));
		if (jobj == NULL)
			break;

		json->jobjs[json->count++] = jobj;
	}

	json_tokener_free(jtok);
}

struct json_object *nemojson_search_object(struct nemojson *json, int index, int depth, ...)
{
	struct json_object *jobj;
	const char *key;
	va_list vargs;
	int i;

	jobj = json->jobjs[index];
	if (jobj == NULL)
		return NULL;

	va_start(vargs, depth);

	for (i = 0; i < depth; i++) {
		key = va_arg(vargs, const char *);
		if ((uint64_t)key < NEMOJSON_ARRAY_MAX) {
			if (json_object_is_type(jobj, json_type_array) == 0)
				goto nofound;
			if ((jobj = json_object_array_get_idx(jobj, (uint64_t)key)) == NULL)
				goto nofound;
		} else {
			if (json_object_is_type(jobj, json_type_object) == 0)
				goto nofound;
			if (json_object_object_get_ex(jobj, key, &jobj) == 0)
				goto nofound;
		}
	}

	va_end(vargs);

	return jobj;

nofound:
	va_end(vargs);

	return NULL;
}
