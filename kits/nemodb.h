#ifndef	__NEMO_DB_H__
#define	__NEMO_DB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemoitem.h>

extern struct nemodb *nemodb_create(const char *uri);
extern void nemodb_destroy(struct nemodb *db);

extern int nemodb_use_collection(struct nemodb *db, const char *dbname, const char *dbcollection);
extern int nemodb_drop_collection(struct nemodb *db);

extern int nemodb_insert_one(struct nemodb *db, const char *key, const char *value);
extern int nemodb_insert_many(struct nemodb *db, struct itemone *one);

extern int nemodb_remove_one(struct nemodb *db, const char *key, const char *value);
extern int nemodb_remove_many(struct nemodb *db, struct itemone *one);

extern char *nemodb_query_one_by_one(struct nemodb *db, const char *key, const char *value, const char *name);
extern char *nemodb_query_one_by_many(struct nemodb *db, struct itemone *one, const char *name);
extern struct itemone *nemodb_query_many_by_one(struct nemodb *db, const char *key, const char *value);
extern struct itemone *nemodb_query_many_by_many(struct nemodb *db, struct itemone *one);
extern struct dbiter *nemodb_query_iter_by_one(struct nemodb *db, const char *key, const char *value);
extern struct dbiter *nemodb_query_iter_by_many(struct nemodb *db, struct itemone *one);
extern struct dbiter *nemodb_query_iter_all(struct nemodb *db);

extern struct itemone *nemodb_iter_next(struct dbiter *iter);

extern int nemodb_modify_many_to_many(struct nemodb *db, struct itemone *sone, struct itemone *done);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
