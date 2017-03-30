#ifndef	__NEMO_NOTY_H__
#define	__NEMO_NOTY_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <nemolist.h>

typedef int (*nemonoty_dispatch_t)(void *data, void *event);

struct notyone {
	struct nemolist link;

	nemonoty_dispatch_t dispatch;
	void *data;
};

struct nemonoty {
	struct nemolist list;
};

extern struct nemonoty *nemonoty_create(void);
extern void nemonoty_destroy(struct nemonoty *noty);

extern void nemonoty_attach(struct nemonoty *noty, nemonoty_dispatch_t dispatch, void *data);
extern void nemonoty_detach(struct nemonoty *noty, nemonoty_dispatch_t dispatch, void *data);

extern void nemonoty_dispatch(struct nemonoty *noty, void *event);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
