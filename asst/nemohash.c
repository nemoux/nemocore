#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <nemohash.h>
#include <codehelper.h>

static uint64_t nemohash_get_tag(struct nemohash *hash, uint64_t key)
{
	key += (key << 12);
	key ^= (key >> 22);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 2);
	key += (key << 7);
	key ^= (key >> 12);

	key = (key >> 3) * 2654435761;

	return key % hash->tablesize;
}

struct nemohash *nemohash_create(int chainlength)
{
	struct nemohash *hash;

	hash = (struct nemohash *)malloc(sizeof(struct nemohash));
	if (hash == NULL)
		return NULL;

	hash->tablesize = NEMOHASH_INITIAL_SIZE;
	hash->chainlength = NEMOHASH_CHAIN_LENGTH_MIN > chainlength ? NEMOHASH_CHAIN_LENGTH_MIN : chainlength;
	hash->nodecount = 0;

	hash->nodes = (struct hashnode *)malloc(sizeof(struct hashnode) * hash->tablesize);
	if (hash->nodes == NULL)
		goto err1;
	memset(hash->nodes, 0, sizeof(struct hashnode) * hash->tablesize);

	return hash;

err1:
	free(hash);

	return NULL;
}

void nemohash_destroy(struct nemohash *hash)
{
	free(hash->nodes);
	free(hash);
}

int nemohash_length(struct nemohash *hash)
{
	return hash->nodecount;
}

int nemohash_clear(struct nemohash *hash)
{
	memset(hash->nodes, 0, sizeof(struct hashnode) * hash->tablesize);

	hash->nodecount = 0;

	return 0;
}

static int nemohash_expand(struct nemohash *hash)
{
	struct hashnode *cnodes;
	struct hashnode *tnodes;
	struct hashnode *node;
	int oldsize;
	int i;

	tnodes = (struct hashnode *)malloc(sizeof(struct hashnode) * hash->tablesize * 2);
	if (tnodes == NULL)
		return -1;
	memset(tnodes, 0, sizeof(struct hashnode) * hash->tablesize * 2);

	cnodes = hash->nodes;
	hash->nodes = tnodes;

	oldsize = hash->tablesize;
	hash->tablesize = hash->tablesize * 2;
	hash->nodecount = 0;

	for (i = 0; i < oldsize; i++) {
		node = &cnodes[i];

		if (node->used != 0) {
			nemohash_set_value(hash, node->key, node->value);
		}
	}

	free(cnodes);

	return 0;
}

void nemohash_iterate(struct nemohash *hash, void (*iterate)(void *data, uint64_t key, uint64_t value), void *data)
{
	int i;

	for (i = 0; i < hash->tablesize; i++) {
		if (hash->nodes[i].used != 0) {
			iterate(data, hash->nodes[i].key, hash->nodes[i].value);
		}
	}
}

int nemohash_set_value(struct nemohash *hash, uint64_t key, uint64_t value)
{
	struct hashnode *node;
	uint64_t tag;
	int i;

retry:
	if (hash->nodecount >= hash->tablesize / 2)
		nemohash_expand(hash);

	tag = nemohash_get_tag(hash, key);

	for (i = 0; i < hash->chainlength; i++) {
		node = &hash->nodes[tag];

		if ((node->used == 0) ||
				(node->used != 0 && node->key == key)) {
			node->key = key;
			node->value = value;
			node->used = 1;

			hash->nodecount++;

			return 1;
		}

		tag = (tag + 1) % hash->tablesize;
	}

	if (nemohash_expand(hash) == 0)
		goto retry;

	return 0;
}

int nemohash_get_value(struct nemohash *hash, uint64_t key, uint64_t *value)
{
	uint64_t tag;
	int i;

	tag = nemohash_get_tag(hash, key);

	for (i = 0; i < hash->chainlength; i++) {
		if (hash->nodes[tag].used != 0 && hash->nodes[tag].key == key) {
			*value = hash->nodes[tag].value;
			return 1;
		}

		tag = (tag + 1) % hash->tablesize;
	}

	return 0;
}

uint64_t nemohash_get_value_easy(struct nemohash *hash, uint64_t key)
{
	uint64_t tag;
	int i;

	tag = nemohash_get_tag(hash, key);

	for (i = 0; i < hash->chainlength; i++) {
		if (hash->nodes[tag].used != 0 && hash->nodes[tag].key == key) {
			return hash->nodes[tag].value;
		}

		tag = (tag + 1) % hash->tablesize;
	}

	return 0;
}

int nemohash_put_value(struct nemohash *hash, uint64_t key)
{
	struct hashnode *node;
	uint64_t tag;
	int i;

	tag = nemohash_get_tag(hash, key);

	for (i = 0; i < hash->chainlength; i++) {
		node = &hash->nodes[tag];

		if (node->used != 0 && node->key == key) {
			node->key = 0;
			node->value = 0;
			node->used = 0;

			hash->nodecount--;

			return 1;
		}

		tag = (tag + 1) % hash->tablesize;
	}

	return 0;
}

int nemohash_put_value_all(struct nemohash *hash, uint64_t value)
{
	int i;

	for (i = 0; i < hash->tablesize; i++) {
		if (hash->nodes[i].used != 0 && hash->nodes[i].value == value) {
			struct hashnode *node = &hash->nodes[i];

			node->key = 0;
			node->value = 0;
			node->used = 0;

			hash->nodecount--;
		}
	}

	return 0;
}

int nemohash_set_value_with_string(struct nemohash *hash, const char *str, uint64_t value)
{
	uint64_t key = crc32_from_string(str);

	return nemohash_set_value(hash, key, value);
}

int nemohash_get_value_with_string(struct nemohash *hash, const char *str, uint64_t *value)
{
	uint64_t key = crc32_from_string(str);

	return nemohash_get_value(hash, key, value);
}

uint64_t nemohash_get_value_easy_with_string(struct nemohash *hash, const char *str)
{
	if (str != NULL) {
		uint64_t key = crc32_from_string(str);

		return nemohash_get_value_easy(hash, key);
	}

	return 0;
}

int nemohash_put_value_with_string(struct nemohash *hash, const char *str)
{
	uint64_t key = crc32_from_string(str);

	return nemohash_put_value(hash, key);
}
