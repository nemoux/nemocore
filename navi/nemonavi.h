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

	void *cc;
};

extern int nemonavi_init_once(int argc, char *argv[]);
extern void nemonavi_exit_once(void);
extern void nemonavi_loop_once(void);

extern struct nemonavi *nemonavi_create(const char *url);
extern void nemonavi_destroy(struct nemonavi *navi);

extern void nemonavi_set_size(struct nemonavi *navi, int32_t width, int32_t height);

static inline void nemonavi_set_dispatch_paint(struct nemonavi *navi, nemonavi_dispatch_paint_t dispatch)
{
	navi->dispatch_paint = dispatch;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
