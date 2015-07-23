#ifndef	__FB_BACKEND_H__
#define	__FB_BACKEND_H__

#include <libudev.h>
#include <compz.h>
#include <backend.h>

struct fbbackend {
	struct nemobackend base;

	struct nemocompz *compz;

	struct udev *udev;
};

extern struct nemobackend *fbbackend_create(struct nemocompz *compz, const char *rendernode);
extern void fbbackend_destroy(struct nemobackend *base);

#endif
