#ifndef __NEMOACTION_TAP_H__
#define __NEMOACTION_TAP_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <math.h>

#include <nemolist.h>

struct nemoaction;
struct actiontap;

typedef int (*nemoaction_tap_dispatch_event_t)(struct nemoaction *action, struct actiontap *tap, uint32_t event);

struct actiontap {
	struct nemoaction *action;

	uint64_t device;
	uint32_t serial;

	float dx, dy;
	float dd;

	float gx0, gy0;
	float gx, gy;
	float lx0, ly0;
	float lx, ly;
	float tx0, ty0;
	float tx, ty;

	uint32_t time0;
	uint32_t time;

	void *target;

	nemoaction_tap_dispatch_event_t dispatch_event;

	struct nemolist link;
};

extern struct actiontap *nemoaction_tap_create(struct nemoaction *action);
extern void nemoaction_tap_destroy(struct actiontap *tap);

extern int nemoaction_tap_set_focus(struct actiontap *tap, void *target);

static inline void nemoaction_tap_set_grab_gx(struct actiontap *tap, float gx)
{
	tap->gx0 = gx;
}

static inline void nemoaction_tap_set_grab_gy(struct actiontap *tap, float gy)
{
	tap->gy0 = gy;
}

static inline void nemoaction_tap_set_gx(struct actiontap *tap, float gx)
{
	tap->gx = gx;
}

static inline void nemoaction_tap_set_gy(struct actiontap *tap, float gy)
{
	tap->gy = gy;
}

static inline void nemoaction_tap_set_grab_lx(struct actiontap *tap, float lx)
{
	tap->lx0 = lx;
}

static inline void nemoaction_tap_set_grab_ly(struct actiontap *tap, float ly)
{
	tap->ly0 = ly;
}

static inline void nemoaction_tap_set_lx(struct actiontap *tap, float lx)
{
	tap->lx = lx;
}

static inline void nemoaction_tap_set_ly(struct actiontap *tap, float ly)
{
	tap->ly = ly;
}

static inline void nemoaction_tap_set_grab_tx(struct actiontap *tap, float tx)
{
	tap->tx0 = tx;
}

static inline void nemoaction_tap_set_grab_ty(struct actiontap *tap, float ty)
{
	tap->ty0 = ty;
}

static inline void nemoaction_tap_set_tx(struct actiontap *tap, float tx)
{
	tap->tx = tx;
}

static inline void nemoaction_tap_set_ty(struct actiontap *tap, float ty)
{
	tap->ty = ty;
}

static inline void nemoaction_tap_set_grab_time(struct actiontap *tap, uint32_t msecs)
{
	tap->time0 = msecs;
}

static inline void nemoaction_tap_set_time(struct actiontap *tap, uint32_t msecs)
{
	tap->time = msecs;
}

static inline void nemoaction_tap_set_device(struct actiontap *tap, uint64_t device)
{
	tap->device = device;
}

static inline void nemoaction_tap_set_serial(struct actiontap *tap, uint64_t serial)
{
	tap->serial = serial;
}

static inline float nemoaction_tap_get_grab_gx(struct actiontap *tap)
{
	return tap->gx0;
}

static inline float nemoaction_tap_get_grab_gy(struct actiontap *tap)
{
	return tap->gy0;
}

static inline float nemoaction_tap_get_gx(struct actiontap *tap)
{
	return tap->gx;
}

static inline float nemoaction_tap_get_gy(struct actiontap *tap)
{
	return tap->gy;
}

static inline float nemoaction_tap_get_grab_lx(struct actiontap *tap)
{
	return tap->lx0;
}

static inline float nemoaction_tap_get_grab_ly(struct actiontap *tap)
{
	return tap->ly0;
}

static inline float nemoaction_tap_get_lx(struct actiontap *tap)
{
	return tap->lx;
}

static inline float nemoaction_tap_get_ly(struct actiontap *tap)
{
	return tap->ly;
}

static inline float nemoaction_tap_get_grab_tx(struct actiontap *tap)
{
	return tap->tx0;
}

static inline float nemoaction_tap_get_grab_ty(struct actiontap *tap)
{
	return tap->ty0;
}

static inline float nemoaction_tap_get_tx(struct actiontap *tap)
{
	return tap->tx;
}

static inline float nemoaction_tap_get_ty(struct actiontap *tap)
{
	return tap->ty;
}

static inline uint32_t nemoaction_tap_get_grab_time(struct actiontap *tap)
{
	return tap->time0;
}

static inline uint32_t nemoaction_tap_get_time(struct actiontap *tap)
{
	return tap->time;
}

static inline uint32_t nemoaction_tap_get_device(struct actiontap *tap)
{
	return tap->device;
}

static inline uint32_t nemoaction_tap_get_serial(struct actiontap *tap)
{
	return tap->serial;
}

static inline void nemoaction_tap_clear(struct actiontap *tap, float x, float y)
{
	tap->dx = x;
	tap->dy = y;
	tap->dd = 0.0f;
}

static inline void nemoaction_tap_trace(struct actiontap *tap, float x, float y)
{
	float dx = x - tap->dx;
	float dy = y - tap->dy;

	tap->dd = sqrtf(dx * dx + dy * dy);
	tap->dx = x;
	tap->dy = y;
}

static inline float nemoaction_tap_get_distance(struct actiontap *tap)
{
	return tap->dd;
}

static inline void nemoaction_tap_set_callback(struct actiontap *tap, nemoaction_tap_dispatch_event_t dispatch)
{
	tap->dispatch_event = dispatch;
}

static inline int nemoaction_tap_dispatch_event(struct nemoaction *action, struct actiontap *tap, uint32_t event)
{
	return tap->dispatch_event(action, tap, event);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
