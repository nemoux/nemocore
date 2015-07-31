#ifndef	__NEMOSHOW_FONT_H__
#define	__NEMOSHOW_FONT_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#include <hb-ft.h>
#include <hb-ot.h>

#include <fonthelper.h>

typedef enum {
	NEMOSHOW_NORMAL_LAYOUT = 0,
	NEMOSHOW_HARFBUZZ_LAYOUT = 1,
	NEMOSHOW_LAST_LAYOUT
} NemoShowFontLayout;

struct showfont {
	char *path;

	int layout;

	double upem;
	double max_advance_height;

	hb_font_t *hbfont;

	void *cc;
};

extern struct showfont *nemoshow_font_create(void);
extern void nemoshow_font_destroy(struct showfont *font);

extern int nemoshow_font_load(struct showfont *font, const char *path);
extern int nemoshow_font_use_harfbuzz(struct showfont *font);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
