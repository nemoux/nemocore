#ifndef	__NEMO_DB_H__
#define	__NEMO_DB_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern struct nemodb *nemodb_create(const char *uri);
extern void nemodb_destroy(struct nemodb *db);

extern int nemodb_use_collection(struct nemodb *db, const char *dbname, const char *dbcollection);
extern int nemodb_drop_collection(struct nemodb *db);

extern struct json_object *nemodb_load_json_object(struct nemodb *db);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
