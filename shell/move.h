#ifndef	__NEMO_SHELL_MOVE_H__
#define	__NEMO_SHELL_MOVE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <grab.h>

struct nemoshell;
struct shellbin;

struct shellgrab_move {
	struct shellgrab base;
	struct touchgrab touch;

	float dx, dy;
};

extern int nemoshell_move_canvas_by_pointer(struct nemoshell *shell, struct nemopointer *pointer, struct shellbin *bin);
extern int nemoshell_move_canvas_by_touchpoint(struct nemoshell *shell, struct touchpoint *tp, struct shellbin *bin);
extern int nemoshell_move_canvas(struct nemoshell *shell, struct shellbin *bin, uint32_t serial);

extern int nemoshell_move_canvas_force(struct nemoshell *shell, struct touchpoint *tp, struct shellbin *bin);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
