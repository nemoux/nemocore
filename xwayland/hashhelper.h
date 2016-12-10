#ifndef	__HASH_HELPER_H__
#define	__HASH_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define HASH_INITIAL_SIZE			(256)
#define HASH_CHAIN_LENGTH_MIN	(32)

struct hashnode {
	uint64_t key;
	int used;
	uint64_t value;
};

struct hash {
	int tablesize;
	int chainlength;
	int nodecount;

	struct hashnode *nodes;
};

extern struct hash *hash_create(int chainlength);
extern void hash_destroy(struct hash *hash);

extern int hash_length(struct hash *hash);
extern int hash_clear(struct hash *hash);

extern void hash_iterate(struct hash *hash, void (*iterate)(void *data, uint64_t key, uint64_t value), void *data);

extern int hash_set_value(struct hash *hash, uint64_t key, uint64_t value);
extern int hash_get_value(struct hash *hash, uint64_t key, uint64_t *value);
extern uint64_t hash_get_value_easy(struct hash *hash, uint64_t key);
extern int hash_put_value(struct hash *hash, uint64_t key);
extern int hash_put_value_all(struct hash *hash, uint64_t value);

extern int hash_set_value_with_string(struct hash *hash, const char *str, uint64_t value);
extern int hash_get_value_with_string(struct hash *hash, const char *str, uint64_t *value);
extern uint64_t hash_get_value_easy_with_string(struct hash *hash, const char *str);
extern int hash_put_value_with_string(struct hash *hash, const char *str);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
