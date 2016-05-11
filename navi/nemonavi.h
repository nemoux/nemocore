#ifndef	__NEMONAVI_H__
#define	__NEMONAVI_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

typedef enum {
	NEMONAVI_PAINT_VIEW_TYPE = 0,
	NEMONAVI_PAINT_POPUP_TYPE = 1,
	NEMONAVI_PAINT_LAST_TYPE
} NemoNaviPaintType;

struct nemonavi;

typedef void (*nemonavi_dispatch_paint_t)(struct nemonavi *navi, int type, const void *buffer, int width, int height, int dx, int dy, int dw, int dh);
typedef void (*nemonavi_dispatch_popup_show_t)(struct nemonavi *navi, int show);
typedef void (*nemonavi_dispatch_popup_rect_t)(struct nemonavi *navi, int x, int y, int width, int height);
typedef void (*nemonavi_dispatch_key_event_t)(struct nemonavi *navi, uint32_t code, int focus_on_editable_field);
typedef void (*nemonavi_dispatch_loading_state_t)(struct nemonavi *navi, int is_loading, int can_go_back, int can_go_forward);

struct nemonavi {
	nemonavi_dispatch_paint_t dispatch_paint;
	nemonavi_dispatch_popup_show_t dispatch_popup_show;
	nemonavi_dispatch_popup_rect_t dispatch_popup_rect;
	nemonavi_dispatch_key_event_t dispatch_key_event;
	nemonavi_dispatch_loading_state_t dispatch_loading_state;

	int32_t width, height;

	uint32_t touchids;

	void *data;

	void *cc;
};

extern int nemonavi_init_once(int argc, char *argv[]);
extern void nemonavi_exit_once(void);

extern void nemonavi_do_message(void);

extern struct nemonavi *nemonavi_create(const char *url);
extern void nemonavi_destroy(struct nemonavi *navi);

extern void nemonavi_set_size(struct nemonavi *navi, int32_t width, int32_t height);

extern void nemonavi_send_pointer_enter_event(struct nemonavi *navi, float x, float y);
extern void nemonavi_send_pointer_leave_event(struct nemonavi *navi, float x, float y);
extern void nemonavi_send_pointer_down_event(struct nemonavi *navi, float x, float y, int button);
extern void nemonavi_send_pointer_up_event(struct nemonavi *navi, float x, float y, int button);
extern void nemonavi_send_pointer_motion_event(struct nemonavi *navi, float x, float y);
extern void nemonavi_send_pointer_wheel_event(struct nemonavi *navi, float x, float y);

extern void nemonavi_send_keyboard_down_event(struct nemonavi *navi, uint32_t code, uint32_t sym, uint32_t modifiers);
extern void nemonavi_send_keyboard_up_event(struct nemonavi *navi, uint32_t code, uint32_t sym, uint32_t modifiers);

extern void nemonavi_send_touch_down_event(struct nemonavi *navi, float x, float y, uint32_t id, double secs);
extern void nemonavi_send_touch_up_event(struct nemonavi *navi, float x, float y, uint32_t id, double secs);
extern void nemonavi_send_touch_motion_event(struct nemonavi *navi, float x, float y, uint32_t id, double secs);
extern void nemonavi_send_touch_cancel_event(struct nemonavi *navi, float x, float y, uint32_t id, double secs);

extern void nemonavi_load_url(struct nemonavi *navi, const char *url);
extern void nemonavi_load_page(struct nemonavi *navi, const char *path);

extern void nemonavi_go_back(struct nemonavi *navi);
extern int nemonavi_can_go_back(struct nemonavi *navi);
extern void nemonavi_go_forward(struct nemonavi *navi);
extern int nemonavi_can_go_forward(struct nemonavi *navi);
extern void nemonavi_reload(struct nemonavi *navi);

extern double nemonavi_get_zoomlevel(struct nemonavi *navi);
extern void nemonavi_set_zoomlevel(struct nemonavi *navi, double zoomlevel);

static inline void nemonavi_set_dispatch_paint(struct nemonavi *navi, nemonavi_dispatch_paint_t dispatch)
{
	navi->dispatch_paint = dispatch;
}

static inline void nemonavi_set_dispatch_popup_show(struct nemonavi *navi, nemonavi_dispatch_popup_show_t dispatch)
{
	navi->dispatch_popup_show = dispatch;
}

static inline void nemonavi_set_dispatch_popup_rect(struct nemonavi *navi, nemonavi_dispatch_popup_rect_t dispatch)
{
	navi->dispatch_popup_rect = dispatch;
}

static inline void nemonavi_set_dispatch_key_event(struct nemonavi *navi, nemonavi_dispatch_key_event_t dispatch)
{
	navi->dispatch_key_event = dispatch;
}

static inline void nemonavi_set_dispatch_loading_state(struct nemonavi *navi, nemonavi_dispatch_loading_state_t dispatch)
{
	navi->dispatch_loading_state = dispatch;
}

static inline int nemonavi_get_width(struct nemonavi *navi)
{
	return navi->width;
}

static inline int nemonavi_get_height(struct nemonavi *navi)
{
	return navi->height;
}

static inline void nemonavi_set_userdata(struct nemonavi *navi, void *data)
{
	navi->data = data;
}

static inline void *nemonavi_get_userdata(struct nemonavi *navi)
{
	return navi->data;
}

static inline uint32_t nemonavi_get_touchid_empty(struct nemonavi *navi)
{
	return ffs(navi->touchids);
}

static inline void nemonavi_set_touchid(struct nemonavi *navi, uint32_t id)
{
	navi->touchids |= (1 << id);
}

static inline void nemonavi_put_touchid(struct nemonavi *navi, uint32_t id)
{
	navi->touchids &= ~(1 << id);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
