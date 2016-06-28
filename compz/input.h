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
	
	uint32_t sampling;

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
extern int nemoinput_set_transform(struct inputnode *node, const char *cmd);
extern void nemoinput_set_sampling(struct inputnode *node, uint32_t sampling);

extern void nemoinput_transform_to_global(struct inputnode *node, float dx, float dy, float *x, float *y);
extern void nemoinput_transform_from_global(struct inputnode *node, float x, float y, float *dx, float *dy);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
