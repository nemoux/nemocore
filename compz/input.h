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
	struct wl_listener screen_destroy_listener;
};

extern void nemoinput_set_screen(struct inputnode *node, struct nemoscreen *screen);
extern void nemoinput_put_screen(struct inputnode *node);

extern int nemoinput_get_config_screen(struct nemocompz *compz, const char *devnode, uint32_t *nodeid, uint32_t *screenid);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
