#ifndef	__NEMO_INPUT_H__
#define	__NEMO_INPUT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemomatrix.h>

struct nemocompz;
struct nemoscreen;

struct inputnode {
	char *devnode;

	struct nemoscreen *screen;

	int32_t x, y, width, height;

	struct {
		int enable;

		struct nemomatrix matrix;
		struct nemomatrix inverse;
	} transform;

	struct wl_listener screen_destroy_listener;
};

extern void nemoinput_set_screen(struct inputnode *node, struct nemoscreen *screen);
extern void nemoinput_put_screen(struct inputnode *node);

extern void nemoinput_set_geometry(struct inputnode *node, int32_t x, int32_t y, int32_t width, int32_t height);

extern int nemoinput_get_config_screen(struct nemocompz *compz, const char *devnode, uint32_t *nodeid, uint32_t *screenid);
extern int nemoinput_get_config_geometry(struct nemocompz *compz, const char *devnode, struct inputnode *node);
extern uint32_t nemoinput_get_config_sampling(struct nemocompz *compz, const char *devnode);

extern void nemoinput_transform_to_global(struct inputnode *node, float dx, float dy, float *x, float *y);
extern void nemoinput_transform_from_global(struct inputnode *node, float x, float y, float *dx, float *dy);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
