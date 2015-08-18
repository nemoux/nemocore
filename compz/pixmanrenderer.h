#ifndef	__PIXMAN_RENDERER_H__
#define	__PIXMAN_RENDERER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <pixman.h>

struct nemocompz;
struct nemorenderer;
struct nemoscreen;
struct rendernode;

extern struct nemorenderer *pixmanrenderer_create(struct rendernode *node);
extern void pixmanrenderer_destroy(struct nemorenderer *renderer);

extern int pixmanrenderer_prepare_screen(struct nemorenderer *base, struct nemoscreen *screen);
extern void pixmanrenderer_finish_screen(struct nemorenderer *base, struct nemoscreen *screen);
extern void pixmanrenderer_set_screen_buffer(struct nemorenderer *base, struct nemoscreen *screen, pixman_image_t *buffer);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
