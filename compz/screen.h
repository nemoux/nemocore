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
	NEMO_DPMS_ON_STATE = 0,
	NEMO_DPMS_STANDBY_STATE = 1,
	NEMO_DPMS_SUSPEND_STATE = 2,
	NEMO_DPMS_OFF_STATE = 3,
	NEMO_DPMS_LAST_STATE
} NemoDpmsState;

struct nemocompz;
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

	uint32_t screenid;
	uint32_t id;
	char name[64];

	char *make, *model, *serial;

	int repaint_needed;
	int repaint_scheduled;
	uint32_t frame_msecs;
	uint32_t frame_count;

	uint64_t msc;

	int use_pixman;
	int32_t x, y, width, height;
	int32_t mmwidth, mmheight;
	uint32_t subpixel;

	int snddev;

	struct {
		struct nemomatrix matrix;
	} render;

	struct {
		float px, py;
		int has_pivot;

		float x, y;
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

extern void nemoscreen_prepare(struct nemoscreen *screen);
extern void nemoscreen_finish(struct nemoscreen *screen);

extern void nemoscreen_update_geometry(struct nemoscreen *screen);
extern void nemoscreen_transform_to_global(struct nemoscreen *screen, float dx, float dy, float *x, float *y);
extern void nemoscreen_transform_from_global(struct nemoscreen *screen, float x, float y, float *dx, float *dy);

extern void nemoscreen_set_position(struct nemoscreen *screen, float x, float y);
extern void nemoscreen_set_rotation(struct nemoscreen *screen, float r);
extern void nemoscreen_set_scale(struct nemoscreen *screen, float sx, float sy);
extern void nemoscreen_set_pivot(struct nemoscreen *screen, float px, float py);

extern void nemoscreen_damage_dirty(struct nemoscreen *screen);

extern int nemoscreen_get_config_mode(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, struct nemomode *mode);
extern int nemoscreen_get_config_geometry(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, struct nemoscreen *screen);
extern const char *nemoscreen_get_config(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, const char *attr);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
