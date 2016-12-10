#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <hashhelper.h>
#include <codehelper.h>

static uint64_t hash_get_tag(struct hash *hash, uint64_t key)
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

struct hash *hash_create(int chainlength)
{
	struct hash *hash;

	hash = (struct hash *)malloc(sizeof(struct hash));
	if (hash == NULL)
		return NULL;

	hash->tablesize = HASH_INITIAL_SIZE;
	hash->chainlength = HASH_CHAIN_LENGTH_MIN > chainlength ? HASH_CHAIN_LENGTH_MIN : chainlength;
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

void hash_destroy(struct hash *hash)
{
	free(hash->nodes);
	free(hash);
}

int hash_length(struct hash *hash)
{
	return hash->nodecount;
}

int hash_clear(struct hash *hash)
{
	memset(hash->nodes, 0, sizeof(struct hashnode) * hash->tablesize);

	hash->nodecount = 0;

	return 0;
}

static int hash_expand(struct hash *hash)
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
			hash_set_value(hash, node->key, node->value);
		}
	}

	free(cnodes);

	return 0;
}

void hash_iterate(struct hash *hash, void (*iterate)(void *data, uint64_t key, uint64_t value), void *data)
{
	int i;

	for (i = 0; i < hash->tablesize; i++) {
		if (hash->nodes[i].used != 0) {
			iterate(data, hash->nodes[i].key, hash->nodes[i].value);
		}
	}
}

int hash_set_value(struct hash *hash, uint64_t key, uint64_t value)
{
	struct hashnode *node;
	uint64_t tag;
	int i;

retry:
	if (hash->nodecount >= hash->tablesize / 2)
		hash_expand(hash);

	tag = hash_get_tag(hash, key);

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

	if (hash_expand(hash) == 0)
		goto retry;

	return 0;
}

int hash_get_value(struct hash *hash, uint64_t key, uint64_t *value)
{
	uint64_t tag;
	int i;

	tag = hash_get_tag(hash, key);

	for (i = 0; i < hash->chainlength; i++) {
		if (hash->nodes[tag].used != 0 && hash->nodes[tag].key == key) {
			*value = hash->nodes[tag].value;
			return 1;
		}

		tag = (tag + 1) % hash->tablesize;
	}

	return 0;
}

uint64_t hash_get_value_easy(struct hash *hash, uint64_t key)
{
	uint64_t tag;
	int i;

	tag = hash_get_tag(hash, key);

	for (i = 0; i < hash->chainlength; i++) {
		if (hash->nodes[tag].used != 0 && hash->nodes[tag].key == key) {
			return hash->nodes[tag].value;
		}

		tag = (tag + 1) % hash->tablesize;
	}

	return 0;
}

int hash_put_value(struct hash *hash, uint64_t key)
{
	struct hashnode *node;
	uint64_t tag;
	int i;

	tag = hash_get_tag(hash, key);

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

int hash_put_value_all(struct hash *hash, uint64_t value)
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

int hash_set_value_with_string(struct hash *hash, const char *str, uint64_t value)
{
	uint64_t key = crc32_from_string(str);

	return hash_set_value(hash, key, value);
}

int hash_get_value_with_string(struct hash *hash, const char *str, uint64_t *value)
{
	uint64_t key = crc32_from_string(str);

	return hash_get_value(hash, key, value);
}

uint64_t hash_get_value_easy_with_string(struct hash *hash, const char *str)
{
	if (str != NULL) {
		uint64_t key = crc32_from_string(str);

		return hash_get_value_easy(hash, key);
	}

	return 0;
}

int hash_put_value_with_string(struct hash *hash, const char *str)
{
	uint64_t key = crc32_from_string(str);

	return hash_put_value(hash, key);
}
