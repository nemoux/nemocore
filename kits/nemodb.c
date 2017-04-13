#define _GNU_SOURCE
#define __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <mongoc.h>
#include <json.h>

#include <nemodb.h>
#include <nemomisc.h>

struct nemodb {
	mongoc_client_t *client;
	mongoc_collection_t *collection;
};

void __attribute__((constructor(101))) nemodb_initialize(void)
{
	mongoc_init();
}

void __attribute__((destructor(101))) nemodb_finalize(void)
{
	mongoc_cleanup();
}

struct nemodb *nemodb_create(const char *uri)
{
	struct nemodb *db;

	db = (struct nemodb *)malloc(sizeof(struct nemodb));
	if (db == NULL)
		return NULL;
	memset(db, 0, sizeof(struct nemodb));

	db->client = mongoc_client_new(uri);
	if (db->client == NULL)
		goto err1;

	return db;

err1:
	free(db);

	return NULL;
}

void nemodb_destroy(struct nemodb *db)
{
	if (db->collection != NULL)
		mongoc_collection_destroy(db->collection);

	mongoc_client_destroy(db->client);

	free(db);
}

int nemodb_use_collection(struct nemodb *db, const char *dbname, const char *dbcollection)
{
	if (db->collection != NULL)
		mongoc_collection_destroy(db->collection);

	db->collection = mongoc_client_get_collection(db->client, dbname, dbcollection);
	if (db->collection == NULL)
		return -1;

	return 0;
}

int nemodb_drop_collection(struct nemodb *db)
{
	if (db->collection != NULL) {
		mongoc_collection_destroy(db->collection);

		db->collection = NULL;
	}

	return 0;
}

struct json_object *nemodb_load_json_object(struct nemodb *db)
{
	struct json_object *jobj = NULL;
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_t *bson;
	char *jstr;

	bson = bson_new();

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_next(cursor, &bdoc)) {
		jstr = bson_as_json(bdoc, NULL);

		jobj = json_tokener_parse(jstr);

		bson_free(jstr);
	}

	mongoc_cursor_destroy(cursor);

	return jobj;
}
