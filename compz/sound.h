#ifndef	__NEMO_SOUND_H__
#define	__NEMO_SOUND_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemosound {
	struct nemocompz *compz;

	struct wl_signal destroy_signal;

	struct wl_list resource_list;
};

extern struct nemosound *nemosound_create(struct nemocompz *compz);
extern void nemosound_destroy(struct nemosound *sound);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
