#ifndef	__TUIO_BACKEND_H__
#define	__TUIO_BACKEND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <backend.h>

struct tuiobackend {
	struct nemobackend base;

	struct nemocompz *compz;
};

extern struct nemobackend *tuiobackend_create(struct nemocompz *compz);
extern void tuiobackend_destroy(struct nemobackend *base);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
