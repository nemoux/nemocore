#ifndef	__NEMO_SCREEN_H__
#define	__NEMO_SCREEN_H__

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

	uint64_t msc;

	int use_pixman;
	int32_t x, y, width, height;
	int32_t r;
	int32_t mmwidth, mmheight;
	int32_t pwidth, pheight;
	uint32_t subpixel;

	int32_t diagonal;

	char *snddev;

	struct {
		struct nemomatrix matrix;
	} render;

	struct {
		struct nemomatrix matrix;
		struct nemomatrix inverse;
	} transform;

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
	struct wl_signal frame_signal;
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

extern void nemoscreen_prepare(struct nemoscreen *screen, int32_t x, int32_t y, int32_t width, int32_t height, int32_t mmwidth, int32_t mmheight, int32_t r, int32_t pwidth, int32_t pheight, int32_t diagonal);
extern void nemoscreen_finish(struct nemoscreen *screen);

extern void nemoscreen_update_geometry(struct nemoscreen *screen);
extern void nemoscreen_transform_to_global(struct nemoscreen *screen, float dx, float dy, float *x, float *y);
extern void nemoscreen_transform_from_global(struct nemoscreen *screen, float x, float y, float *dx, float *dy);

extern int nemoscreen_get_config_mode(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, struct nemomode *mode);
extern int nemoscreen_get_config_geometry(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, int32_t *x, int32_t *y, int32_t *width, int32_t *height, int32_t *r, int32_t *diagonal);
extern const char *nemoscreen_get_config(struct nemocompz *compz, uint32_t nodeid, uint32_t screenid, const char *attr);

#endif
