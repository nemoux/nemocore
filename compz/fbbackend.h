#ifndef	__FB_BACKEND_H__
#define	__FB_BACKEND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <libudev.h>
#include <compz.h>
#include <backend.h>

struct fbbackend {
	struct nemobackend base;

	struct nemocompz *compz;

	struct udev *udev;
};

extern struct nemobackend *fbbackend_create(struct nemocompz *compz, const char *args);
extern void fbbackend_destroy(struct nemobackend *base);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
