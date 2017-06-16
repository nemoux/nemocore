#ifndef	__NEMO_SCREEN_H__
#define	__NEMO_SCREEN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>
#include <pixman.h>

#include <nemomatrix.h>

typedef enum {
	NEMOSCREEN_DISPLAY_STATE = (1 << 0),
	NEMOSCREEN_SCOPE_STATE = (1 << 1),
	NEMOSCREEN_OVERLAY_STATE = (1 << 2),
	NEMOSCREEN_LAST_STATE
} NemoScreenState;

typedef enum {
	NEMODPMS_ON_STATE = 0,
	NEMODPMS_STANDBY_STATE = 1,
	NEMODPMS_SUSPEND_STATE = 2,
	NEMODPMS_OFF_STATE = 3,
	NEMODPMS_LAST_STATE
} NemoDpmsState;

struct nemocompz;
struct nemoview;
struct rendernode;

struct nemomode {
	uint32_t flags;
	int32_t width, height;
	uint32_t refresh;

	struct wl_list link;
};

struct nemoscreen {
	struct nemocompz *compz;
	struct rendernode *node;

	uint32_t state;

	uint32_t screenid;
	uint32_t id;
	char name[64];

	char *make, *model, *serial;

	int repaint_needed;
	int repaint_scheduled;
	uint32_t frame_msecs;
	uint32_t frame_count;

	struct wl_event_source *frameout_timer;
	uint32_t frameout_timeout;
	int frameout_scheduled;
	int frameout_needed;

	uint64_t msc;

	int32_t x, y;
	int32_t width, height;
	int32_t mmwidth, mmheight;
	uint32_t subpixel;

	struct {
		struct nemomatrix matrix;
	} render;

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

	struct nemoview *overlay;
	struct wl_listener overlay_destroy_listener;

	int32_t rx, ry, rw, rh;

	pixman_region32_t region;
	pixman_region32_t damage;
	void *pcontext;
	void *gcontext;

	pixman_format_code_t read_format;

	struct nemomode *current_mode;
	struct wl_list mode_list;

	uint16_t gamma_size;

	struct wl_list link;
	struct wl_list resource_list;
	struct wl_signal destroy_signal;
	struct wl_global *global;

	void (*repaint)(struct nemoscreen *base);
	int (*repaint_frame)(struct nemoscreen *base, pixman_region32_t *damage);
	int (*read_pixels)(struct nemoscreen *screen, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void (*destroy)(struct nemoscreen *base);
	void (*set_dpms)(struct nemoscreen *base, int dpms);
	int (*switch_mode)(struct nemoscreen *base, struct nemomode *mode);
	void (*set_gamma)(struct nemoscreen *base, uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b);
};

extern void nemoscreen_schedule_repaint(struct nemoscreen *screen);
extern void nemoscreen_finish_frame(struct nemoscreen *screen, uint32_t secs, uint32_t usecs, uint32_t psf_flags);
extern int nemoscreen_read_pixels(struct nemoscreen *screen, pixman_format_code_t format, void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
extern int nemoscreen_switch_mode(struct nemoscreen *screen, int32_t width, int32_t height, uint32_t refresh);

extern void nemoscreen_prepare(struct nemoscreen *screen);
extern void nemoscreen_finish(struct nemoscreen *screen);

extern void nemoscreen_transform_to_global(struct nemoscreen *screen, float dx, float dy, float *x, float *y);
extern void nemoscreen_transform_from_global(struct nemoscreen *screen, float x, float y, float *dx, float *dy);

extern void nemoscreen_clear_transform(struct nemoscreen *screen);
extern void nemoscreen_update_transform(struct nemoscreen *screen);

extern void nemoscreen_set_position(struct nemoscreen *screen, int32_t x, int32_t y);
extern void nemoscreen_set_rotation(struct nemoscreen *screen, float r);
extern void nemoscreen_set_scale(struct nemoscreen *screen, float sx, float sy);
extern void nemoscreen_set_pivot(struct nemoscreen *screen, float px, float py);
extern int nemoscreen_set_custom(struct nemoscreen *screen, const char *cmd);

extern int nemoscreen_set_frameout(struct nemoscreen *screen, uint32_t timeout);

extern int nemoscreen_set_overlay(struct nemoscreen *screen, struct nemoview *view);

extern void nemoscreen_transform_dirty(struct nemoscreen *screen);
extern void nemoscreen_damage_dirty(struct nemoscreen *screen);

static inline void nemoscreen_set_state(struct nemoscreen *screen, uint32_t state)
{
	screen->state |= state;
}

static inline void nemoscreen_put_state(struct nemoscreen *screen, uint32_t state)
{
	screen->state &= ~state;
}

static inline int nemoscreen_has_state(struct nemoscreen *screen, uint32_t state)
{
	return screen->state & state;
}

static inline int nemoscreen_has_state_all(struct nemoscreen *screen, uint32_t state)
{
	return (screen->state & state) == state;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
