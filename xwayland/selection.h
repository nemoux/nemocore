#ifndef	__NEMO_XWAYLAND_SELECTION_H__
#define	__NEMO_XWAYLAND_SELECTION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

extern void nemoxmanager_init_selection(struct nemoxmanager *xmanager);
extern void nemoxmanager_exit_selection(struct nemoxmanager *xmanager);

extern int nemoxmanager_handle_selection_event(struct nemoxmanager *xmanager, xcb_generic_event_t *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
