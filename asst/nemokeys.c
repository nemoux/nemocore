#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemokeys.h>
#include <nemomisc.h>

struct nemokeys *nemokeys_create(const char *path)
{
	struct nemokeys *keys;
	leveldb_options_t *options;
	char *err = NULL;

	keys = (struct nemokeys *)malloc(sizeof(struct nemokeys));
	if (keys == NULL)
		return NULL;
	memset(keys, 0, sizeof(struct nemokeys));

	options = leveldb_options_create();
	leveldb_options_set_create_if_missing(options, 1);
	leveldb_options_set_info_log(options, NULL);
	leveldb_options_set_write_buffer_size(options, 100000);
	leveldb_options_set_paranoid_checks(options, 1);
	leveldb_options_set_max_open_files(options, 10);
	leveldb_options_set_block_size(options, 1024);
	leveldb_options_set_block_restart_interval(options, 8);
	leveldb_options_set_compression(options, leveldb_no_compression);

	keys->roptions = leveldb_readoptions_create();
	leveldb_readoptions_set_verify_checksums(keys->roptions, 1);
	leveldb_readoptions_set_fill_cache(keys->roptions, 0);

	keys->woptions = leveldb_writeoptions_create();
	leveldb_writeoptions_set_sync(keys->woptions, 1);

	keys->db = leveldb_open(options, path, &err);
	if (err != NULL) {
		fprintf(stderr, "failed to open db: %s\n", err);
		leveldb_free(err);
		goto err1;
	}

	return keys;

err1:
	leveldb_readoptions_destroy(keys->roptions);
	leveldb_writeoptions_destroy(keys->woptions);

	leveldb_options_destroy(options);

	free(keys);

	return NULL;
}

void nemokeys_destroy(struct nemokeys *keys)
{
	leveldb_readoptions_destroy(keys->roptions);
	leveldb_writeoptions_destroy(keys->woptions);

	leveldb_close(keys->db);

	free(keys);
}

int nemokeys_set(struct nemokeys *keys, const char *key, const char *value)
{
	char *err = NULL;

	leveldb_put(keys->db, keys->woptions, key, strlen(key), value, strlen(value), &err);
	if (err != NULL) {
		fprintf(stderr, "failed to set key/value: %s\n", err);
		leveldb_free(err);
		return -1;
	}

	return 0;
}

char *nemokeys_get(struct nemokeys *keys, const char *key)
{
	char *value;
	char *err = NULL;
	size_t length;

	value = leveldb_get(keys->db, keys->roptions, key, strlen(key), &length, &err);
	if (err != NULL) {
		fprintf(stderr, "failed to get key/value: %s\n", err);
		leveldb_free(err);
		return NULL;
	}

	return value;
}

struct keysiter *nemokeys_create_iterator(struct nemokeys *keys)
{
	struct keysiter *iter;

	iter = (struct keysiter *)malloc(sizeof(struct keysiter));
	if (iter == NULL)
		return NULL;
	memset(iter, 0, sizeof(struct keysiter));

	iter->iter = leveldb_create_iterator(keys->db, keys->roptions);

	return iter;
}

void nemokeys_destroy_iterator(struct keysiter *iter)
{
	leveldb_iter_destroy(iter->iter);

	free(iter);
}

int nemokeys_iterator_seek_to_first(struct keysiter *iter)
{
	leveldb_iter_seek_to_first(iter->iter);

	return leveldb_iter_valid(iter->iter) != 0;
}

int nemokeys_iterator_seek_to_last(struct keysiter *iter)
{
	leveldb_iter_seek_to_last(iter->iter);

	return leveldb_iter_valid(iter->iter) != 0;
}

int nemokeys_iterator_seek(struct keysiter *iter, const char *key)
{
	leveldb_iter_seek(iter->iter, key, strlen(key));

	return leveldb_iter_valid(iter->iter) != 0;
}

int nemokeys_iterator_next(struct keysiter *iter)
{
	leveldb_iter_next(iter->iter);

	return leveldb_iter_valid(iter->iter) != 0;
}

int nemokeys_iterator_prev(struct keysiter *iter)
{
	leveldb_iter_prev(iter->iter);

	return leveldb_iter_valid(iter->iter) != 0;
}

const char *nemokeys_iterator_key(struct keysiter *iter)
{
	size_t length;

	return leveldb_iter_key(iter->iter, &length);
}

const char *nemokeys_iterator_value(struct keysiter *iter)
{
	size_t length;

	return leveldb_iter_value(iter->iter, &length);
}
