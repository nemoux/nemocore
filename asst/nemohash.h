#ifndef	__NEMO_HASH_H__
#define	__NEMO_HASH_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#define NEMOHASH_INITIAL_SIZE			(256)
#define NEMOHASH_CHAIN_LENGTH_MIN	(32)

struct hashnode {
	uint64_t key;
	int used;
	uint64_t value;
};

struct nemohash {
	int tablesize;
	int chainlength;
	int nodecount;

	struct hashnode *nodes;
};

extern struct nemohash *nemohash_create(int chainlength);
extern void nemohash_destroy(struct nemohash *hash);

extern int nemohash_length(struct nemohash *hash);
extern int nemohash_clear(struct nemohash *hash);

extern void nemohash_iterate(struct nemohash *hash, void (*iterate)(void *data, uint64_t key, uint64_t value), void *data);

extern int nemohash_set_value(struct nemohash *hash, uint64_t key, uint64_t value);
extern int nemohash_get_value(struct nemohash *hash, uint64_t key, uint64_t *value);
extern uint64_t nemohash_get_value_easy(struct nemohash *hash, uint64_t key);
extern int nemohash_put_value(struct nemohash *hash, uint64_t key);
extern int nemohash_put_value_all(struct nemohash *hash, uint64_t value);

extern int nemohash_set_value_with_string(struct nemohash *hash, const char *str, uint64_t value);
extern int nemohash_get_value_with_string(struct nemohash *hash, const char *str, uint64_t *value);
extern uint64_t nemohash_get_value_easy_with_string(struct nemohash *hash, const char *str);
extern int nemohash_put_value_with_string(struct nemohash *hash, const char *str);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
