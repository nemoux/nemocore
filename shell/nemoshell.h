#ifndef	__NEMO_NEMO_SHELL_H__
#define	__NEMO_NEMO_SHELL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern int nemoshell_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

extern int nemoshell_is_nemo_surface(struct shellbin *bin);
extern int nemoshell_is_nemo_surface_for_canvas(struct nemocanvas *canvas);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
