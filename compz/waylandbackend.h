#ifndef	__WAYLAND_BACKEND_H__
#define	__WAYLAND_BACKEND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <compz.h>
#include <backend.h>

struct waylandbackend {
	struct nemobackend base;

	struct nemocompz *compz;
};

extern struct nemobackend *waylandbackend_create(struct nemocompz *compz);
extern void waylandbackend_destroy(struct nemobackend *base);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
