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
	NEMOSCOPE_LAST_TYPE
} NemoScopeType;

struct scopeone {
	float x, y;
	float w, h;

	uint32_t tag;

	int type;

	struct nemolist link;
};

struct nemoscope {
	struct nemolist list;
};

extern struct nemoscope *nemoscope_create(void);
extern void nemoscope_destroy(struct nemoscope *scope);

extern void nemoscope_clear(struct nemoscope *scope);

extern int nemoscope_add(struct nemoscope *scope, uint32_t tag, uint32_t type, float x, float y, float w, float h);
extern uint32_t nemoscope_pick(struct nemoscope *scope, float x, float y);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
