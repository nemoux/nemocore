#ifndef	__NEMO_BACKEND_H__
#define	__NEMO_BACKEND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemobackend {
	struct wl_list link;

	void (*destroy)(struct nemobackend *base);
};

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
