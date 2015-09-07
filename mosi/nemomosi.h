#ifndef	__NEMOMOSI_H__
#define	__NEMOMOSI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <mosirandom.h>
#include <mosiwave.h>
#include <mosiflip.h>
#include <mosirain.h>
#include <mosicross.h>
#include <mosioneshot.h>

struct mosione {
	uint8_t c[4];
	uint8_t c0[4];
	uint8_t c1[4];

	int has_transition;
	int done_transition;
	uint32_t stime, etime;
};

struct nemomosi {
	int32_t width, height;

	struct mosione *ones;
};

extern struct nemomosi *nemomosi_create(int32_t width, int32_t height);
extern void nemomosi_destroy(struct nemomosi *mosi);

extern void nemomosi_clear_one(struct nemomosi *mosi, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
extern void nemomosi_tween_color(struct nemomosi *mosi, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
extern void nemomosi_tween_image(struct nemomosi *mosi, uint8_t *c);

extern int nemomosi_update(struct nemomosi *mosi, uint32_t msecs);

extern int nemomosi_get_empty(struct nemomosi *mosi, int *x, int *y);

static inline int nemomosi_get_width(struct nemomosi *mosi)
{
	return mosi->width;
}

static inline int nemomosi_get_height(struct nemomosi *mosi)
{
	return mosi->height;
}

static inline struct mosione *nemomosi_get_one(struct nemomosi *mosi, int x, int y)
{
	return &mosi->ones[y * mosi->width + x];
}

static inline uint8_t nemomosi_one_get_a(struct mosione *one)
{
	return one->c[3];
}

static inline uint8_t nemomosi_one_get_r(struct mosione *one)
{
	return one->c[2];
}

static inline uint8_t nemomosi_one_get_g(struct mosione *one)
{
	return one->c[1];
}

static inline uint8_t nemomosi_one_get_b(struct mosione *one)
{
	return one->c[0];
}

static inline void nemomosi_one_set_transition(struct mosione *one, uint32_t stime, uint32_t dtime)
{
	if (one->done_transition == 0) {
		if (one->has_transition == 0 || one->stime > stime) {
			one->stime = stime;
			one->etime = stime + dtime;

			one->has_transition = 1;
		}
	}
}

#ifdef __cplusplus
}
#endif

#endif
