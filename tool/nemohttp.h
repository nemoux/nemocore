#ifndef	__NEMOTOOL_HTTP_H__
#define	__NEMOTOOL_HTTP_H__

#define	NEMOHTTP_MAX_FIELDS		(64)
#define	NEMOHTTP_MAX_COOKIES	(64)

#define	NEMOHTTP_COOKIE_NAME_SIZE		(32)
#define	NEMOHTTP_COOKIE_VALUE_SIZE	(256)

struct nemohttp {
	char *status;
	char *url;
	char *body;

	char *fields[NEMOHTTP_MAX_FIELDS][2];
	int nfields;

	char *cookies[NEMOHTTP_MAX_COOKIES][2];
	int ncookies;
	int has_cookies;
};

extern struct nemohttp *nemohttp_create(const char *data, int length);
extern void nemohttp_destroy(struct nemohttp *http);

extern const char *nemohttp_get_status(struct nemohttp *http);
extern const char *nemohttp_get_url(struct nemohttp *http);
extern const char *nemohttp_get_body(struct nemohttp *http);
extern const char *nemohttp_get_field(struct nemohttp *http, const char *name, int *nfield);
extern const char *nemohttp_get_cookie(struct nemohttp *http, const char *name);

extern int nemohttp_need_follow(struct nemohttp *http);
extern struct nemohttp *nemohttp_follow(struct nemohttp *http);

#endif
