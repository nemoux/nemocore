#ifndef	__NEMOPLAY_H__
#define	__NEMOPLAY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

struct nemoplay {
	void *userdata;
};

extern struct nemoplay *nemoplay_create(void);
extern void nemoplay_destroy(struct nemoplay *play);

static inline void nemoplay_set_userdata(struct nemoplay *play, void *userdata)
{
	play->userdata = userdata;
}

static inline void *nemoplay_get_userdata(struct nemoplay *play)
{
	return play->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
