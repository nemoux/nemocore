#ifndef	__NEMO_JSON_H__
#define	__NEMO_JSON_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoitem.h>

#define NEMOJSON_OBJECT_MAX			(128)

struct json_object;

struct nemojson {
	char *contents;
	int length;

	struct json_object *jobjs[NEMOJSON_OBJECT_MAX];
	char *jkeys[NEMOJSON_OBJECT_MAX];
	int count;
};

extern struct nemojson *nemojson_create_string(const char *str, int length);
extern struct nemojson *nemojson_create_format(const char *fmt, ...);
extern struct nemojson *nemojson_create_file(const char *filepath);
extern struct nemojson *nemojson_create(void);
extern void nemojson_destroy(struct nemojson *json);

extern int nemojson_append(struct nemojson *json, const char *str, int length);
extern int nemojson_append_one(struct nemojson *json, char c);
extern int nemojson_append_format(struct nemojson *json, const char *fmt, ...);
extern int nemojson_append_file(struct nemojson *json, const char *filepath);

extern int nemojson_update(struct nemojson *json);

extern int nemojson_insert_object(struct nemojson *json, const char *jkey, struct json_object *jobj);
extern int nemojson_iterate_object(struct nemojson *json, struct json_object *jobj);

extern struct json_object *nemojson_search_object(struct nemojson *json, int index, int depth, ...);
extern int nemojson_search_integer(struct nemojson *json, int index, int value, int depth, ...);
extern double nemojson_search_double(struct nemojson *json, int index, double value, int depth, ...);
extern const char *nemojson_search_string(struct nemojson *json, int index, const char *value, int depth, ...);

extern struct json_object *nemojson_search_attribute(struct json_object *jobj, const char *key, const char *value);

extern struct json_object *nemojson_get_object(struct nemojson *json, int index);
extern const char *nemojson_get_string(struct nemojson *json, int index);
extern double nemojson_get_double(struct nemojson *json, int index);
extern int nemojson_get_integer(struct nemojson *json, int index);
extern int nemojson_get_boolean(struct nemojson *json, int index);

extern struct json_object *nemojson_object_create_file(const char *filepath);

extern struct json_object *nemojson_object_get_object(struct json_object *jobj, const char *name, struct json_object *value);
extern const char *nemojson_object_get_string(struct json_object *jobj, const char *name, const char *value);
extern char *nemojson_object_dup_string(struct json_object *jobj, const char *name, const char *value);
extern double nemojson_object_get_double(struct json_object *jobj, const char *name, double value);
extern int nemojson_object_get_integer(struct json_object *jobj, const char *name, int value);
extern int nemojson_object_get_boolean(struct json_object *jobj, const char *name, int value);

extern int nemojson_array_get_length(struct json_object *jobj);
extern struct json_object *nemojson_array_get_object(struct json_object *jobj, int index);
extern const char *nemojson_array_get_string(struct json_object *jobj, int index);
extern double nemojson_array_get_double(struct json_object *jobj, int index);
extern int nemojson_array_get_integer(struct json_object *jobj, int index);
extern int nemojson_array_get_boolean(struct json_object *jobj, int index);

extern int nemojson_object_load_item(struct json_object *jobj, struct nemoitem *item, const char *prefix);
extern int nemojson_string_load_item(const char *contents, struct nemoitem *item, const char *prefix);
extern int nemojson_object_load_item_one(struct json_object *jobj, struct itemone *one);
extern int nemojson_string_load_item_one(const char *contents, struct itemone *one);

static inline int nemojson_get_count(struct nemojson *json)
{
	return json->count;
}

static inline const char *nemojson_get_key(struct nemojson *json, int index)
{
	return json->jkeys[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
