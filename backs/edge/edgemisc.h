#ifndef __NEMOUX_EDGE_MISC_H__
#define __NEMOUX_EDGE_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

typedef enum {
	EDGEBACK_NONE_SITE = 0,
	EDGEBACK_TOP_SITE = 1,
	EDGEBACK_BOTTOM_SITE = 2,
	EDGEBACK_LEFT_SITE = 3,
	EDGEBACK_RIGHT_SITE = 4,
	EDGEBACK_LAST_SITE
} EdgeBackSite;

extern int nemoback_edge_get_site(struct edgeback *edge, double x, double y);
extern double nemoback_edge_get_site_rotation(struct edgeback *edge, int site);
extern int nemoback_edge_get_edge(struct edgeback *edge, double x, double y, uint32_t edgesize);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
