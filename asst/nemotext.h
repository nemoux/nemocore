#ifndef	__NEMO_TEXT_H__
#define	__NEMO_TEXT_H__

#include <stdint.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <hb-ft.h>
#include <hb-ot.h>

#include <nemolist.h>

#define	NEMOTEXT_FONTPATH_MAX			(128)
#define	NEMOTEXT_FONTFAMILY_MAX		(32)
#define	NEMOTEXT_FONTSTYLE_MAX		(64)

struct nemoglyph {
	struct nemofont *font;

	hb_direction_t hbdirection;
	hb_script_t hbscript;
	hb_language_t hblanguage;
	hb_buffer_t *hbbuffer;

	double fontsize;

	cairo_glyph_t *glyphs;
	int nglyphs;
	double width, height;
};

struct nemofont {
	char fontpath[NEMOTEXT_FONTPATH_MAX];
	char fontfamily[NEMOTEXT_FONTFAMILY_MAX];
	char fontstyle[NEMOTEXT_FONTSTYLE_MAX];
	int fontslant;
	int fontweight;
	int fontwidth;
	int fontspacing;

	hb_font_t *hbfont;
	FT_Face ftface;
	cairo_scaled_font_t *cfont;

	double upem;
	double fontscale;

	struct nemolist link;
};

struct nemotext {
	FT_Library ftlibrary;
	FcConfig *fcconfig;

	struct nemolist font_list;
};

extern struct nemotext *nemotext_create(void);
extern void nemotext_destroy(struct nemotext *text);

extern struct nemofont *nemotext_get_font_with_style(struct nemotext *text, const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing);
extern struct nemofont *nemotext_get_font(struct nemotext *text, const char *fontpath);

extern struct nemofont *nemotext_font_create(struct nemotext *text, const char *fontpath);
extern void nemotext_font_destroy(struct nemofont *font);

extern struct nemoglyph *nemotext_glyph_create(struct nemofont *font);
extern void nemotext_glyph_destroy(struct nemoglyph *glyph);
extern int nemotext_glyph_build(struct nemoglyph *glyph, const char *msg, int length);
extern void nemotext_glyph_render(cairo_t *cr, struct nemoglyph *glyph);

static inline void nemotext_glyph_set_direction(struct nemoglyph *glyph, hb_direction_t hbdirection)
{
	glyph->hbdirection = hbdirection;
}

static inline void nemotext_glyph_set_script(struct nemoglyph *glyph, hb_script_t hbscript)
{
	glyph->hbscript = hbscript;
}

static inline void nemotext_glyph_set_language(struct nemoglyph *glyph, hb_language_t hblanguage)
{
	glyph->hblanguage = hblanguage;
}

static inline void nemotext_glyph_set_fontsize(struct nemoglyph *glyph, double fontsize)
{
	glyph->fontsize = fontsize;
}

static inline double nemotext_glyph_get_width(struct nemoglyph *glyph)
{
	return glyph->width;
}

static inline double nemotext_glyph_get_height(struct nemoglyph *glyph)
{
	return glyph->height;
}

#endif
