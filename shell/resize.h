#ifndef	__NEMO_SHELL_RESIZE_H__
#define	__NEMO_SHELL_RESIZE_H__

#include <grab.h>

struct nemoshell;
struct shellbin;

struct shellgrab_resize {
	struct shellgrab base;
	uint32_t edges;
	int32_t width, height;
};

extern int nemoshell_resize_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial, uint32_t edges);

#endif
