#ifndef	__NEMO_INPUT_H__
#define	__NEMO_INPUT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemomatrix.h>

typedef enum {
	NEMOINPUT_POINTER_TYPE = (1 << 0),
	NEMOINPUT_KEYBOARD_TYPE = (1 << 1),
	NEMOINPUT_TOUCH_TYPE = (1 << 2)
} NemoInputType;

typedef enum {
	NEMOINPUT_CONFIG_STATE = (1 << 0)
} NemoInputState;

struct nemocompz;
struct nemoscreen;

struct inputnode {
	char *devnode;

	uint32_t type;

	uint32_t state;

	struct wl_list link;

	struct nemoscreen *screen;

	int32_t x, y, width, height;

	struct {
		float px, py;
		float r;
		float sx, sy;
	} geometry;

	struct {
		int enable;
		int dirty;
		int custom;

		float cosr, sinr;

		struct nemomatrix matrix;
		struct nemomatrix inverse;
	} transform;

	struct wl_listener screen_destroy_listener;
};

extern void nemoinput_set_screen(struct inputnode *node, struct nemoscreen *screen);
extern void nemoinput_put_screen(struct inputnode *node);

extern void nemoinput_clear_transform(struct inputnode *node);
extern void nemoinput_update_transform(struct inputnode *node);

extern void nemoinput_set_size(struct inputnode *node, int32_t width, int32_t height);
extern void nemoinput_set_position(struct inputnode *node, int32_t x, int32_t y);
extern void nemoinput_set_rotation(struct inputnode *node, float r);
extern void nemoinput_set_scale(struct inputnode *node, float sx, float sy);
extern void nemoinput_set_pivot(struct inputnode *node, float px, float py);
extern int nemoinput_set_custom(struct inputnode *node, const char *cmd);

extern void nemoinput_transform_to_global(struct inputnode *node, float dx, float dy, float *x, float *y);
extern void nemoinput_transform_from_global(struct inputnode *node, float x, float y, float *dx, float *dy);

static inline int nemoinput_has_type(struct inputnode *node, uint32_t type)
{
	return (node->type & type) == type;
}

static inline void nemoinput_set_state(struct inputnode *node, uint32_t state)
{
	node->state |= state;
}

static inline void nemoinput_put_state(struct inputnode *node, uint32_t state)
{
	node->state &= ~state;
}

static inline int nemoinput_has_state(struct inputnode *node, uint32_t state)
{
	return node->state & state;
}

static inline int nemoinput_has_state_all(struct inputnode *node, uint32_t state)
{
	return (node->state & state) == state;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
