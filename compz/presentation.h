#ifndef	__NEMO_PRESENTATION_H__
#define	__NEMO_PRESENTATION_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemoscreen;

extern int nemopresentation_bind(struct wl_client *client, void *data, uint32_t version, uint32_t id);

extern void nemopresentation_discard_feedback_list(struct wl_list *list);
extern void nemopresentation_present_feedback_list(struct wl_list *list, struct nemoscreen *screen, uint32_t refresh, uint32_t secs, uint32_t nsecs, uint64_t seq, uint32_t psf_flags);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
