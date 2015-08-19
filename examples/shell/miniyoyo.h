#ifndef	__MINISHELL_YOYO_H__
#define	__MINISHELL_YOYO_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <minishell.h>

#include <showhelper.h>

struct miniyoyo {
	struct showone *path;

	double x0, y0;
	double x1, y1;
	double x2, y2;
};

extern struct miniyoyo *minishell_yoyo_create(void);
extern void minishell_yoyo_destroy(struct miniyoyo *yoyo);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
