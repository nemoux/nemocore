#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <skiaconfig.hpp>

#include <showfont.h>
#include <showfont.hpp>
#include <nemoshow.h>
#include <fonthelper.h>
#include <nemomisc.h>

struct showone *nemoshow_font_create(void)
{
	struct showfont *font;
	struct showone *one;

	font = (struct showfont *)malloc(sizeof(struct showfont));
	if (font == NULL)
		return NULL;
	memset(font, 0, sizeof(struct showfont));

	font->cc = new showfont_t;

	one = &font->base;
	one->type = NEMOSHOW_FONT_TYPE;
	one->update = nemoshow_font_update;
	one->destroy = nemoshow_font_destroy;

	nemoshow_one_prepare(one);

	return one;
}

void nemoshow_font_destroy(struct showone *one)
{
	struct showfont *font = NEMOSHOW_FONT(one);

	nemoshow_one_finish(one);

	if (NEMOSHOW_FONT_CC(font, face) != NULL)
		SkSafeUnref(NEMOSHOW_FONT_CC(font, face));

	delete static_cast<showfont_t *>(font->cc);

	if (font->path != NULL)
		free(font->path);

	if (font->hbfont != NULL)
		hb_font_destroy(font->hbfont);

	free(font);
}

int nemoshow_font_update(struct showone *one)
{
	struct showfont *font = NEMOSHOW_FONT(one);

	return 0;
}

int nemoshow_font_load(struct showone *one, const char *path)
{
	struct showfont *font = NEMOSHOW_FONT(one);

	font->path = strdup(path);

	NEMOSHOW_FONT_CC(font, face) = SkTypeface::CreateFromFile(path, 0);

	font->upem = NEMOSHOW_FONT_CC(font, face)->getUnitsPerEm();
	font->max_advance_height = fontconfig_get_max_advance_height(path, 0);

	nemoshow_one_dirty(one, NEMOSHOW_FONT_DIRTY);

	return 0;
}

int nemoshow_font_load_fontconfig(struct showone *one, const char *fontfamily, const char *fontstyle)
{
	struct showfont *font = NEMOSHOW_FONT(one);
	const char *fontpath;

	fontpath = fontconfig_get_path(
			fontfamily,
			fontstyle,
			FC_SLANT_ROMAN,
			FC_WEIGHT_NORMAL,
			FC_WIDTH_NORMAL,
			FC_MONO);

	nemoshow_font_load(one, fontpath);

	return 0;
}

int nemoshow_font_use_harfbuzz(struct showone *one)
{
	struct showfont *font = NEMOSHOW_FONT(one);

	font->hbfont = fontconfig_create_hb_font(font->path, 0);
	font->layout = NEMOSHOW_HARFBUZZ_LAYOUT;

	return 0;
}
