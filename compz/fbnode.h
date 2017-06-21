#ifndef	__FB_NODE_H__
#define	__FB_NODE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>

#include <compz.h>
#include <backend.h>
#include <screen.h>
#include <renderer.h>
#include <fbhelper.h>

struct nemopointer;
struct nemokeyboard;
struct nemotouch;

struct fbscreen {
	struct nemoscreen base;

	struct fbnode *node;

	struct nemomode mode;

	const char *fbdev;
	struct fbscreeninfo fbinfo;
	void *fbdata;

	pixman_image_t *screen_image;
	pixman_image_t *shadow_image;
	void *shadow_buffer;
	uint8_t depth;
};

struct fbnode {
	struct rendernode base;
};

extern struct fbnode *fb_create_node(struct nemocompz *compz, const char *devpath);
extern void fb_destroy_node(struct fbnode *node);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
