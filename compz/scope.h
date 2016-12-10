#ifndef __NEMO_SCOPE_H__
#define __NEMO_SCOPE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <nemolist.h>

typedef enum {
	NEMOSCOPE_NONE_TYPE = 0,
	NEMOSCOPE_RECT_TYPE = 1,
	NEMOSCOPE_CIRCLE_TYPE = 2,
	NEMOSCOPE_ELLIPSE_TYPE = 3,
	NEMOSCOPE_TRIANGLE_TYPE = 4,
	NEMOSCOPE_POLYGON_TYPE = 5,
	NEMOSCOPE_LAST_TYPE
} NemoScopeType;

struct scopeone {
	int type;

	float *array;
	int arraycount;

	uint32_t tag;

	struct nemolist link;
};

struct nemoscope {
	struct nemolist list;
};

extern struct nemoscope *nemoscope_create(void);
extern void nemoscope_destroy(struct nemoscope *scope);

extern void nemoscope_clear(struct nemoscope *scope);

extern int nemoscope_add_cmd(struct nemoscope *scope, uint32_t tag, const char *cmd);
extern int nemoscope_add_rect(struct nemoscope *scope, uint32_t tag, float x, float y, float w, float h);
extern int nemoscope_add_circle(struct nemoscope *scope, uint32_t tag, float x, float y, float r);
extern int nemoscope_add_triangle(struct nemoscope *scope, uint32_t tag, float x0, float y0, float x1, float y1, float x2, float y2);
extern int nemoscope_add_ellipse(struct nemoscope *scope, uint32_t tag, float x, float y, float rx, float ry);

extern uint32_t nemoscope_pick(struct nemoscope *scope, float x, float y);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
