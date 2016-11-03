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
