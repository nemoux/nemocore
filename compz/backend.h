#ifndef	__NEMO_BACKEND_H__
#define	__NEMO_BACKEND_H__

struct nemobackend {
	struct wl_list link;

	void (*destroy)(struct nemobackend *base);
};

#endif
