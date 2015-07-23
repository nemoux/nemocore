#ifndef	__NEMO_TEXTBACKEND_H__
#define	__NEMO_TEXTBACKEND_H__

#include <pixman.h>

#include <task.h>

struct nemocompz;

struct textbackend {
	struct nemocompz *compz;

	struct {
		char *path;
		struct wl_resource *binding;
		struct nemotask task;
		struct wl_client *client;
		struct wl_listener destroy_listener;

		uint32_t deathcount;
		uint32_t deathstamp;
	} inputmethod;

	struct {
		struct wl_global *global;
	} inputmanager;

	struct wl_listener destroy_listener;
};

extern struct textbackend *textbackend_create(struct nemocompz *compz, const char *inputpath);

#endif
