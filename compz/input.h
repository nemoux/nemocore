#ifndef	__NEMO_INPUT_H__
#define	__NEMO_INPUT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemocompz;
struct nemoscreen;

struct inputnode {
	char *devnode;

	struct nemoscreen *screen;

	int32_t x, y, width, height;

	struct wl_listener screen_destroy_listener;
};

extern void nemoinput_set_screen(struct inputnode *node, struct nemoscreen *screen);
extern void nemoinput_put_screen(struct inputnode *node);

extern void nemoinput_set_geometry(struct inputnode *node, int32_t x, int32_t y, int32_t width, int32_t height);

extern int nemoinput_get_config_screen(struct nemocompz *compz, const char *devnode, uint32_t *nodeid, uint32_t *screenid);
extern int nemoinput_get_config_geometry(struct nemocompz *compz, const char *devnode, int32_t *x, int32_t *y, int32_t *width, int32_t *height);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
