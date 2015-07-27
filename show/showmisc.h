#ifndef	__NEMOSHOW_MISC_H__
#define	__NEMOSHOW_MISC_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

typedef enum {
	NEMOSHOW_NONE_PROP = 0,
	NEMOSHOW_DOUBLE_PROP = 1,
	NEMOSHOW_INTEGER_PROP = 2,
	NEMOSHOW_STRING_PROP = 3,
	NEMOSHOW_COLOR_PROP = 4,
	NEMOSHOW_LAST_PROP
} NemoShowPropType;

struct showprop {
	char name[32];

	int type;
};

extern struct showprop *nemoshow_get_property(const char *name);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
