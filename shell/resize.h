#ifndef	__NEMO_SHELL_RESIZE_H__
#define	__NEMO_SHELL_RESIZE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <grab.h>

struct nemoshell;
struct shellbin;

struct shellgrab_resize {
	struct shellgrab base;
	uint32_t edges;
	int32_t width, height;
};

extern int nemoshell_resize_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial, uint32_t edges);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
