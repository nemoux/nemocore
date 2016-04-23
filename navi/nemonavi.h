#ifndef	__NEMONAVI_H__
#define	__NEMONAVI_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemonavi;

typedef void (*nemonavi_dispatch_paint_t)(struct nemonavi *navi, const void *buffer, int width, int height, int dx, int dy, int dw, int dh);

struct nemonavi {
	nemonavi_dispatch_paint_t dispatch_paint;

	int32_t width, height;

	void *data;

	void *cc;
};

extern int nemonavi_init_once(int argc, char *argv[]);
extern void nemonavi_exit_once(void);
extern void nemonavi_loop_once(void);

extern struct nemonavi *nemonavi_create(const char *url);
extern void nemonavi_destroy(struct nemonavi *navi);

extern void nemonavi_set_size(struct nemonavi *navi, int32_t width, int32_t height);

extern void nemonavi_send_pointer_enter_event(struct nemonavi *navi, float x, float y);
extern void nemonavi_send_pointer_leave_event(struct nemonavi *navi, float x, float y);
extern void nemonavi_send_pointer_down_event(struct nemonavi *navi, float x, float y, int button);
extern void nemonavi_send_pointer_up_event(struct nemonavi *navi, float x, float y, int button);
extern void nemonavi_send_pointer_motion_event(struct nemonavi *navi, float x, float y);

extern void nemonavi_send_keyboard_down_event(struct nemonavi *navi, uint32_t code);
extern void nemonavi_send_keyboard_up_event(struct nemonavi *navi, uint32_t code);

extern void nemonavi_send_touch_down_event(struct nemonavi *navi, float x, float y, uint32_t id);
extern void nemonavi_send_touch_up_event(struct nemonavi *navi, float x, float y, uint32_t id);
extern void nemonavi_send_touch_motion_event(struct nemonavi *navi, float x, float y, uint32_t id);

extern void nemonavi_load_url(struct nemonavi *navi, const char *url);

static inline void nemonavi_set_dispatch_paint(struct nemonavi *navi, nemonavi_dispatch_paint_t dispatch)
{
	navi->dispatch_paint = dispatch;
}

static inline void nemonavi_set_userdata(struct nemonavi *navi, void *data)
{
	navi->data = data;
}

static inline void *nemonavi_get_userdata(struct nemonavi *navi)
{
	return navi->data;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
