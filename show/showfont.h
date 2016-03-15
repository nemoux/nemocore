#ifndef	__NEMOSHOW_FONT_H__
#define	__NEMOSHOW_FONT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <hb-ft.h>
#include <hb-ot.h>

#include <showone.h>

typedef enum {
	NEMOSHOW_NORMAL_LAYOUT = 0,
	NEMOSHOW_HARFBUZZ_LAYOUT = 1,
	NEMOSHOW_LAST_LAYOUT
} NemoShowFontLayout;

struct showfont {
	struct showone base;

	char *path;

	int layout;

	double upem;
	double max_advance_height;

	hb_font_t *hbfont;

	void *cc;
};

#define	NEMOSHOW_FONT(one)					((struct showfont *)container_of(one, struct showfont, base))
#define	NEMOSHOW_FONT_AT(one, at)		(NEMOSHOW_FONT(one)->at)

extern struct showone *nemoshow_font_create(void);
extern void nemoshow_font_destroy(struct showone *one);

extern int nemoshow_font_update(struct showone *one);

extern int nemoshow_font_load(struct showone *one, const char *path);
extern int nemoshow_font_load_fontconfig(struct showone *one, const char *fontfamily, const char *fontstyle);
extern int nemoshow_font_use_harfbuzz(struct showone *one);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
