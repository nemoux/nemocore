#ifndef	__NEMO_NEMO_CLIENT_H__
#define	__NEMO_NEMO_CLIENT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int nemoclient_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
