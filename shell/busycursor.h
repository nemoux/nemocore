#ifndef	__NEMO_BUSY_CURSOR_H__
#define	__NEMO_BUSY_CURSOR_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct shellbin;
struct nemopointer;

extern void nemoshell_start_busycursor_grab(struct shellbin *bin, struct nemopointer *pointer);
extern void nemoshell_end_busycursor_grab(struct nemocompz *compz, struct wl_client *client);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
