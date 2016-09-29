#ifndef	__NEMO_KEYS_H__
#define	__NEMO_KEYS_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <leveldb/c.h>

struct nemokeys {
	leveldb_t *db;
	leveldb_readoptions_t *roptions;
	leveldb_writeoptions_t *woptions;
};

struct keysiter {
	leveldb_iterator_t *iter;
};

extern struct nemokeys *nemokeys_create(const char *path);
extern void nemokeys_destroy(struct nemokeys *keys);

extern void nemokeys_clear(struct nemokeys *keys);

extern int nemokeys_set(struct nemokeys *keys, const char *key, const char *value);
extern char *nemokeys_get(struct nemokeys *keys, const char *key);
extern int nemokeys_put(struct nemokeys *keys, const char *key);

extern struct keysiter *nemokeys_create_iterator(struct nemokeys *keys);
extern void nemokeys_destroy_iterator(struct keysiter *iter);

extern int nemokeys_iterator_seek_to_first(struct keysiter *iter);
extern int nemokeys_iterator_seek_to_last(struct keysiter *iter);
extern int nemokeys_iterator_seek(struct keysiter *iter, const char *key);
extern int nemokeys_iterator_next(struct keysiter *iter);
extern int nemokeys_iterator_prev(struct keysiter *iter);

extern const char *nemokeys_iterator_key(struct keysiter *iter);
extern const char *nemokeys_iterator_value(struct keysiter *iter);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
