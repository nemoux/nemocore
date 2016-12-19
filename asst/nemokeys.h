#ifndef	__NEMO_KEYS_H__
#define	__NEMO_KEYS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <string.h>

struct keysone {
	char *key;
	char *value;
};

struct nemokeys {
	struct keysone *ones;
	int nones, sones;
};

extern struct nemokeys *nemokeys_create(int size);
extern void nemokeys_destroy(struct nemokeys *keys);

extern int nemokeys_set(struct nemokeys *keys, const char *key, const char *value);
extern const char *nemokeys_get(struct nemokeys *keys, const char *key);

extern void nemokeys_update(struct nemokeys *keys);

static inline int nemokeys_get_size(struct nemokeys *keys)
{
	return keys->sones;
}

static inline int nemokeys_get_count(struct nemokeys *keys)
{
	return keys->nones;
}

static inline const char *nemokeys_get_key(struct nemokeys *keys, int index)
{
	return keys->ones[index].key;
}

static inline const char *nemokeys_get_value(struct nemokeys *keys, int index)
{
	return keys->ones[index].value;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
