#ifndef	__NEMO_XWAYLAND_DND_H__
#define	__NEMO_XWAYLAND_DND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern void nemoxmanager_init_dnd(struct nemoxmanager *xmanager);
extern void nemoxmanager_exit_dnd(struct nemoxmanager *xmanager);

extern int nemoxmanager_handle_dnd_event(struct nemoxmanager *xmanager, xcb_generic_event_t *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
