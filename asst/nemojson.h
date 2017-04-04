#ifndef	__NEMO_JSON_H__
#define	__NEMO_JSON_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define NEMOJSON_OBJECT_MAX			(128)

struct json_object;

struct nemojson {
	char *contents;
	int length;

	struct json_object *jobjs[NEMOJSON_OBJECT_MAX];
	int count;
};

extern struct nemojson *nemojson_create(const char *str, int length);
extern struct nemojson *nemojson_create_format(const char *fmt, ...);
extern struct nemojson *nemojson_create_file(const char *filepath);
extern void nemojson_destroy(struct nemojson *json);

extern int nemojson_append(struct nemojson *json, const char *str, int length);
extern int nemojson_append_one(struct nemojson *json, char c);
extern int nemojson_append_format(struct nemojson *json, const char *fmt, ...);

extern void nemojson_update(struct nemojson *json);

extern struct json_object *nemojson_search_object(struct nemojson *json, int index, int depth, ...);
extern int nemojson_search_integer(struct nemojson *json, int index, int value, int depth, ...);
extern double nemojson_search_double(struct nemojson *json, int index, double value, int depth, ...);
extern const char *nemojson_search_string(struct nemojson *json, int index, const char *value, int depth, ...);

static inline int nemojson_get_object_count(struct nemojson *json)
{
	return json->count;
}

static inline struct json_object *nemojson_get_object(struct nemojson *json, int index)
{
	return json->jobjs[index];
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
