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

#include <nemodb.h>
#include <nemomisc.h>

struct dbiter {
	mongoc_cursor_t *cursor;
};

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
	db->collection = mongoc_client_get_collection(db->client, dbname, dbcollection);
	if (db->collection == NULL)
		return -1;

	return 0;
}

int nemodb_drop_collection(struct nemodb *db)
{
	if (mongoc_collection_drop(db->collection, NULL) == 0)
		return -1;

	return 0;
}

static inline bson_t *nemodb_bson_from_ione(struct itemone *one)
{
	struct itemattr *attr;
	bson_t *bson;

	bson = bson_new();

	if (nemoitem_one_get_path(one) != NULL) {
		BSON_APPEND_UTF8(bson,
				"_path",
				nemoitem_one_get_path(one));
	}

	nemoitem_attr_for_each(attr, one) {
		BSON_APPEND_UTF8(bson,
				nemoitem_attr_get_name(attr),
				nemoitem_attr_get_value(attr));
	}

	return bson;
}

static inline struct itemone *nemodb_ione_from_bson(const bson_t *bson)
{
	struct itemone *one;
	bson_iter_t iter;
	const char *key;
	const char *value;

	one = nemoitem_one_create();

	bson_iter_init(&iter, bson);

	while (bson_iter_next(&iter)) {
		key = bson_iter_key(&iter);
		value = bson_iter_utf8(&iter, NULL);

		if (key[0] == '_') {
			if (strcmp(key, "_path") == 0)
				nemoitem_one_set_path(one, value);
		} else {
			nemoitem_one_set_attr(one, key, value);
		}
	}

	return one;
}

int nemodb_insert_one(struct nemodb *db, const char *key, const char *value)
{
	bson_t *bson;
	int r;

	bson = bson_new();
	BSON_APPEND_UTF8(bson, key, value);

	r = mongoc_collection_insert(db->collection, MONGOC_INSERT_NONE, bson, NULL, NULL);

	bson_destroy(bson);

	return r;
}

int nemodb_insert_many(struct nemodb *db, struct itemone *one)
{
	bson_t *bson;
	int r;

	bson = nemodb_bson_from_ione(one);

	r = mongoc_collection_insert(db->collection, MONGOC_INSERT_NONE, bson, NULL, NULL);

	bson_destroy(bson);

	return r;
}

int nemodb_remove_one(struct nemodb *db, const char *key, const char *value)
{
	bson_t *bson;
	int r;

	bson = bson_new();
	BSON_APPEND_UTF8(bson, key, value);

	r = mongoc_collection_remove(db->collection, MONGOC_REMOVE_NONE, bson, NULL, NULL);

	bson_destroy(bson);

	return r;
}

int nemodb_remove_many(struct nemodb *db, struct itemone *one)
{
	bson_t *bson;
	int r;

	bson = nemodb_bson_from_ione(one);

	r = mongoc_collection_remove(db->collection, MONGOC_REMOVE_NONE, bson, NULL, NULL);

	bson_destroy(bson);

	return r;
}

char *nemodb_query_one_by_one(struct nemodb *db, const char *key, const char *value, const char *name)
{
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = bson_new();
	BSON_APPEND_UTF8(bson, key, value);

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_next(cursor, &bdoc)) {
		bson_iter_t iter;

		bson_iter_init(&iter, bdoc);
		if (bson_iter_find(&iter, name)) {
			mongoc_cursor_destroy(cursor);

			return strdup(bson_iter_utf8(&iter, NULL));
		}
	}

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	mongoc_cursor_destroy(cursor);

	return NULL;
}

char *nemodb_query_one_by_many(struct nemodb *db, struct itemone *one, const char *name)
{
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = nemodb_bson_from_ione(one);

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_next(cursor, &bdoc)) {
		bson_iter_t iter;

		bson_iter_init(&iter, bdoc);
		if (bson_iter_find(&iter, name)) {
			mongoc_cursor_destroy(cursor);

			return strdup(bson_iter_utf8(&iter, NULL));
		}
	}

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	mongoc_cursor_destroy(cursor);

	return NULL;
}

struct itemone *nemodb_query_many_by_one(struct nemodb *db, const char *key, const char *value)
{
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = bson_new();
	BSON_APPEND_UTF8(bson, key, value);

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_next(cursor, &bdoc)) {
		mongoc_cursor_destroy(cursor);

		return nemodb_ione_from_bson(bdoc);
	}

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	mongoc_cursor_destroy(cursor);

	return NULL;
}

struct itemone *nemodb_query_many_by_many(struct nemodb *db, struct itemone *one)
{
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = nemodb_bson_from_ione(one);

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_next(cursor, &bdoc)) {
		mongoc_cursor_destroy(cursor);

		return nemodb_ione_from_bson(bdoc);
	}

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	mongoc_cursor_destroy(cursor);

	return NULL;
}

struct dbiter *nemodb_query_iter_by_one(struct nemodb *db, const char *key, const char *value)
{
	struct dbiter *iter;
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = bson_new();
	BSON_APPEND_UTF8(bson, key, value);

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	iter = (struct dbiter *)malloc(sizeof(struct dbiter));
	iter->cursor = cursor;

	return iter;
}

struct dbiter *nemodb_query_iter_by_many(struct nemodb *db, struct itemone *one)
{
	struct dbiter *iter;
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = nemodb_bson_from_ione(one);

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	iter = (struct dbiter *)malloc(sizeof(struct dbiter));
	iter->cursor = cursor;

	return iter;
}

struct dbiter *nemodb_query_iter_all(struct nemodb *db)
{
	struct dbiter *iter;
	mongoc_cursor_t *cursor;
	const bson_t *bdoc;
	bson_error_t berr;
	bson_t *bson;

	bson = bson_new();

	cursor = mongoc_collection_find(db->collection, MONGOC_QUERY_NONE, 0, 0, 0, bson, NULL, NULL);

	bson_destroy(bson);

	if (mongoc_cursor_error(cursor, &berr))
		return NULL;

	iter = (struct dbiter *)malloc(sizeof(struct dbiter));
	iter->cursor = cursor;

	return iter;
}

struct itemone *nemodb_iter_next(struct dbiter *iter)
{
	const bson_t *bson;

	if (mongoc_cursor_next(iter->cursor, &bson) == 0)
		return NULL;

	return nemodb_ione_from_bson(bson);
}

int nemodb_modify_many_to_many(struct nemodb *db, struct itemone *sone, struct itemone *done)
{
	bson_t *qson;
	bson_t *bson;
	bson_t *uson;

	qson = nemodb_bson_from_ione(sone);
	bson = nemodb_bson_from_ione(done);

	uson = bson_new();
	BSON_APPEND_DOCUMENT(uson, "$set", bson);

	if (mongoc_collection_find_and_modify(db->collection, qson, NULL, uson, NULL, false, false, true, NULL, NULL) == 0)
		return -1;

	bson_destroy(uson);
	bson_destroy(qson);

	return 0;
}
