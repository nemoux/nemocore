#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <showfont.h>
#include <showfont.hpp>
#include <nemoshow.h>
#include <nemomisc.h>

struct showfont *nemoshow_font_create(void)
{
	struct showfont *font;

	font = (struct showfont *)malloc(sizeof(struct showfont));
	if (font == NULL)
		return NULL;
	memset(font, 0, sizeof(struct showfont));

	font->cc = new showfont_t;

	return font;
}

void nemoshow_font_destroy(struct showfont *font)
{
	delete static_cast<showfont_t *>(font->cc);

	if (font->path != NULL)
		free(font->path);

	if (font->hbfont != NULL)
		hb_font_destroy(font->hbfont);

	free(font);
}

int nemoshow_font_load(struct showfont *font, const char *path)
{
	font->path = strdup(path);

	NEMOSHOW_FONT_CC(font, face) = SkTypeface::CreateFromFile(path, 0);

	font->upem = NEMOSHOW_FONT_CC(font, face)->getUnitsPerEm();
	font->max_advance_height = fontconfig_get_max_advance_height(path, 0);

	return 0;
}

int nemoshow_font_use_harfbuzz(struct showfont *font)
{
	font->hbfont = fontconfig_create_hb_font(font->path, 0);
	font->layout = NEMOSHOW_HARFBUZZ_LAYOUT;

	return 0;
}
