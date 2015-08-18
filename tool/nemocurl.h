#ifndef	__NEMOTOOL_CURL_H__
#define	__NEMOTOOL_CURL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <curl/curl.h>

#include <nemolist.h>
#include <nemotimer.h>

#define NEMOCURL_MAX_FDS		(64)

typedef enum {
	NEMOCURL_FOLLOW_LOCATION_FLAG = (1 << 0),
} NemoCurlFlag;

struct nemotool;
struct nemocurl;

typedef void (*nemocurl_dispatch_t)(struct nemocurl *curl, uint32_t id, const char *url, const char *header, int nheader, const char *body, int nbody);

struct nemocurl {
	struct nemotool *tool;

	CURLM *handle;

	int fds[NEMOCURL_MAX_FDS];
	int nfds;

	struct nemotask task;
	struct nemotimer *timer;

	nemocurl_dispatch_t callback;

	struct nemolist list;

	void *userdata;
};

struct curlreq {
	CURL *handle;

	struct curl_slist *clist;

	char *url;

	char *header;
	int nheader, sheader;
	char *body;
	int nbody, sbody;

	uint32_t id;

	struct nemolist link;
};

extern struct nemocurl *nemocurl_create(struct nemotool *tool);
extern void nemocurl_destroy(struct nemocurl *curl);

extern void nemocurl_set_callback(struct nemocurl *curl, nemocurl_dispatch_t callback);

extern int nemocurl_request(struct nemocurl *curl, uint32_t id, const char *url, const char *post, uint32_t timeout, uint32_t flags, ...);
extern int nemocurl_dispatch(struct nemocurl *curl);

static inline void nemocurl_set_userdata(struct nemocurl *curl, void *data)
{
	curl->userdata = data;
}

static inline void *nemocurl_get_userdata(struct nemocurl *curl)
{
	return curl->userdata;
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
