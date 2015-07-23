#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <nemotext.h>
#include <nemomisc.h>

struct nemotext *nemotext_create(void)
{
	struct nemotext *text;

	text = (struct nemotext *)malloc(sizeof(struct nemotext));
	if (text == NULL)
		return NULL;
	memset(text, 0, sizeof(struct nemotext));

	if (FT_Init_FreeType(&text->ftlibrary))
		goto err1;

	text->fcconfig = FcInitLoadConfigAndFonts();
	if (!FcConfigSetRescanInterval(text->fcconfig, 0))
		goto err2;

	nemolist_init(&text->font_list);

	return text;

err2:
	FT_Done_FreeType(text->ftlibrary);

err1:
	free(text);

	return NULL;
}

void nemotext_destroy(struct nemotext *text)
{
	struct nemofont *font, *next;

	nemolist_for_each_safe(font, next, &text->font_list, link) {
		nemotext_font_destroy(font);
	}

	nemolist_remove(&text->font_list);

	FcConfigDestroy(text->fcconfig);
	FcFini();

	FT_Done_FreeType(text->ftlibrary);

	free(text);
}

static inline struct nemotext *nemotext_get_instance(void)
{
	static struct nemotext *text = NULL;

	return text != NULL ? text : (text = nemotext_create());
}

static cairo_scaled_font_t *nemotext_create_cairo_font(FT_Face ftface)
{
	cairo_scaled_font_t *cfont;
	cairo_font_face_t *cface;
	cairo_matrix_t umatrix, fmatrix;
	cairo_font_options_t *options;

	cface = cairo_ft_font_face_create_for_ft_face(ftface, 0);
	if (cairo_font_face_status(cface) != CAIRO_STATUS_SUCCESS)
		return NULL;

	cairo_matrix_init_identity(&umatrix);
	cairo_matrix_init_scale(&fmatrix,
			(double)ftface->units_per_EM / (double)ftface->max_advance_height,
			(double)ftface->units_per_EM / (double)ftface->max_advance_height);

	options = cairo_font_options_create();
	cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_NONE);
	cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
	cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_DEFAULT);

	if (cairo_font_options_get_antialias(options) == CAIRO_ANTIALIAS_SUBPIXEL) {
		cairo_font_options_set_subpixel_order(options, CAIRO_SUBPIXEL_ORDER_DEFAULT);
	}

	cfont = cairo_scaled_font_create(cface, &fmatrix, &umatrix, options);

	cairo_font_options_destroy(options);
	cairo_font_face_destroy(cface);

	return cfont;
}

typedef struct hb_file_t hb_file_t;
struct hb_file_t {
	char *data;
	size_t length;
};

static void nemotext_handle_hb_blob_destroy(hb_file_t *hbfile)
{
	if (hbfile != NULL) {
		munmap(hbfile->data, hbfile->length);

		free(hbfile);
	}
}

struct nemofont *nemotext_font_create(struct nemotext *text, const char *fontpath)
{
	struct nemofont *font;
	hb_file_t *hbfile;
	hb_blob_t *hbblob;
	hb_face_t *hbface;
	hb_font_t *hbfont;
	FT_Face ftface;
	cairo_scaled_font_t *cfont;
	char *data;
	size_t length;
	struct stat st;
	double upem;
	int fd;

	if (text == NULL)
		text = nemotext_get_instance();

	fd = open(fontpath, O_RDONLY);
	if (fd < 0)
		return NULL;
	fstat(fd, &st);
	length = st.st_size;
	data = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED) {
		close(fd);
		return NULL;
	}
	close(fd);

	hbfile = (hb_file_t *)malloc(sizeof(hb_file_t));
	if (hbfile == NULL) {
		munmap(data, length);
		return NULL;
	}
	hbfile->data = data;
	hbfile->length = length;

	hbblob = hb_blob_create(data, length,
			HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE,
			hbfile,
			(hb_destroy_func_t)nemotext_handle_hb_blob_destroy);

	hbface = hb_face_create(hbblob, 0);
	hb_blob_destroy(hbblob);
	if (hbface == NULL)
		return NULL;

	upem = hb_face_get_upem(hbface);

	hbfont = hb_font_create(hbface);
	hb_face_destroy(hbface);
	if (hbfont == NULL)
		return NULL;

	hb_font_set_scale(hbfont, upem, upem);
	hb_ot_font_set_funcs(hbfont);

	if (FT_New_Face(text->ftlibrary, fontpath, 0, &ftface))
		goto err1;

	cfont = nemotext_create_cairo_font(ftface);
	if (cfont == NULL)
		goto err2;

	font = (struct nemofont *)malloc(sizeof(struct nemofont));
	if (font == NULL)
		return NULL;
	memset(font, 0, sizeof(struct nemofont));

	font->hbfont = hbfont;
	font->ftface = ftface;
	font->cfont = cfont;
	font->upem = upem;
	font->fontscale = upem / (double)ftface->max_advance_height;

	strcpy(font->fontpath, fontpath);

	nemolist_insert(&text->font_list, &font->link);

	return font;

err2:
	FT_Done_Face(ftface);

err1:
	hb_font_destroy(hbfont);

	return NULL;
}

void nemotext_font_destroy(struct nemofont *font)
{
	nemolist_remove(&font->link);

	FT_Done_Face(font->ftface);

	hb_font_destroy(font->hbfont);

	free(font);
}

static const char *nemotext_get_fontpath_with_style(struct nemotext *text, const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing)
{
	FcPattern *pattern;
	FcPattern *match;
	FcFontSet *set;
	FcChar8 *filepath;
	FcResult res;
	FcBool r;
	char *fontpath;
	int i;

	pattern = FcPatternCreate();

	if (fontfamily != NULL)
		FcPatternAddString(pattern, FC_FAMILY, (unsigned char *)fontfamily);
	if (fontstyle != NULL)
		FcPatternAddString(pattern, FC_STYLE, (unsigned char *)fontstyle);
	if (fontslant >= 0)
		FcPatternAddInteger(pattern, FC_SLANT, fontslant);
	if (fontweight >= 0)
		FcPatternAddInteger(pattern, FC_WEIGHT, fontweight);
	if (fontwidth >= 0)
		FcPatternAddInteger(pattern, FC_WIDTH, fontwidth);
	if (fontspacing >= 0)
		FcPatternAddInteger(pattern, FC_SPACING, fontspacing);

	FcConfigSubstitute(text->fcconfig, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	set = FcFontSetCreate();

	match = FcFontMatch(text->fcconfig, pattern, &res);
	if (match != NULL)
		FcFontSetAdd(set, match);

	if (match == NULL || set->nfont == 0) {
		FcFontSet *tset;

		tset = FcFontSort(text->fcconfig, pattern, FcTrue, NULL, &res);
		if (tset == NULL || tset->nfont == 0)
			goto out;

		for (i = 0; i < tset->nfont; i++) {
			match = FcFontRenderPrepare(NULL, pattern, tset->fonts[i]);
			if (match != NULL)
				FcFontSetAdd(set, match);
		}

		FcFontSetDestroy(tset);
	}

	FcPatternDestroy(pattern);

	if (set->nfont == 0 || set->fonts[0] == NULL)
		goto out;

	r = FcPatternGetString(set->fonts[0], FC_FILE, 0, &filepath);
	if (r != FcResultMatch || filepath == NULL)
		goto out;

	fontpath = strdup(filepath);

out:
	FcFontSetDestroy(set);

	return fontpath;
}

struct nemofont *nemotext_get_font_with_style(struct nemotext *text, const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing)
{
	struct nemofont *font;
	const char *fontpath;

	if (text == NULL)
		text = nemotext_get_instance();

	nemolist_for_each(font, &text->font_list, link) {
		if (fontfamily == NULL || strcmp(font->fontfamily, fontfamily) != 0)
			continue;
		if (fontstyle == NULL || strcmp(font->fontstyle, fontstyle) != 0)
			continue;
		if (font->fontslant != fontslant)
			continue;
		if (font->fontweight != fontweight)
			continue;
		if (font->fontwidth != fontwidth)
			continue;
		if (font->fontspacing != fontspacing)
			continue;

		return font;
	}

	fontpath = nemotext_get_fontpath_with_style(text, fontfamily, fontstyle, fontslant, fontweight, fontwidth, fontspacing);
	if (fontpath == NULL)
		return NULL;

	font = nemotext_font_create(text, fontpath);
	if (font == NULL)
		return NULL;

	if (fontfamily != NULL)
		strcpy(font->fontfamily, fontfamily);
	if (fontstyle != NULL)
		strcpy(font->fontstyle, fontstyle);
	font->fontslant = fontslant;
	font->fontweight = fontweight;
	font->fontwidth = fontwidth;
	font->fontspacing = fontspacing;

	return font;
}

struct nemofont *nemotext_get_font(struct nemotext *text, const char *fontpath)
{
	struct nemofont *font;

	if (text == NULL)
		text = nemotext_get_instance();

	nemolist_for_each(font, &text->font_list, link) {
		if (strcmp(font->fontpath, fontpath) == 0)
			return font;
	}

	return nemotext_font_create(text, fontpath);
}

struct nemoglyph *nemotext_glyph_create(struct nemofont *font)
{
	struct nemoglyph *glyph;

	glyph = (struct nemoglyph *)malloc(sizeof(struct nemoglyph));
	if (glyph == NULL)
		return NULL;
	memset(glyph, 0, sizeof(struct nemoglyph));

	glyph->hbbuffer = hb_buffer_create();
	if (!hb_buffer_allocation_successful(glyph->hbbuffer))
		goto err1;

	glyph->font = font;

	return glyph;

err1:
	free(glyph);

	return NULL;
}

void nemotext_glyph_destroy(struct nemoglyph *glyph)
{
	if (glyph->glyphs != NULL)
		cairo_glyph_free(glyph->glyphs);

	hb_buffer_destroy(glyph->hbbuffer);

	free(glyph);
}

int nemotext_glyph_build(struct nemoglyph *glyph, const char *msg, int length)
{
	struct nemofont *font = glyph->font;
	hb_buffer_t *hbbuffer = glyph->hbbuffer;
	hb_glyph_info_t *hbglyphs;
	hb_glyph_position_t *hbpositions;
	hb_position_t x, y;
	cairo_glyph_t *cglyphs;
	double width, height;
	uint32_t nglyphs;
	int i;

	if (glyph->glyphs != NULL) {
		cairo_glyph_free(glyph->glyphs);
		glyph->glyphs = NULL;
	}

	hb_buffer_clear_contents(glyph->hbbuffer);

	hb_buffer_add_utf8(hbbuffer, msg, length, 0, length);

	hb_buffer_set_direction(hbbuffer, glyph->hbdirection);
	hb_buffer_set_script(hbbuffer, glyph->hbscript);
	hb_buffer_set_language(hbbuffer, glyph->hblanguage);
	hb_buffer_set_flags(hbbuffer, HB_BUFFER_FLAG_DEFAULT);
	hb_buffer_guess_segment_properties(hbbuffer);

	if (!hb_shape_full(font->hbfont, hbbuffer, NULL, 0, NULL))
		return -1;

	hbglyphs = hb_buffer_get_glyph_infos(hbbuffer, &nglyphs);
	if (hbglyphs == NULL)
		return -1;

	cglyphs = cairo_glyph_allocate(nglyphs);
	if (cglyphs == NULL)
		return -1;

	hbpositions = hb_buffer_get_glyph_positions(hbbuffer, NULL);

	for (i = 0, x = 0, y = 0, width = 0.0f, height = 0.0f; i < nglyphs; i++) {
		width += hbpositions[i].x_advance / (double)font->ftface->max_advance_height * glyph->fontsize;
		height -= hbpositions[i].y_advance / (double)font->ftface->max_advance_height * glyph->fontsize;

		cglyphs[i].index = hbglyphs[i].codepoint;
		cglyphs[i].x = (hbpositions[i].x_offset + x) / (double)font->ftface->max_advance_height * glyph->fontsize;
		cglyphs[i].y = (-hbpositions[i].y_offset + y) / (double)font->ftface->max_advance_height * glyph->fontsize;

		x += hbpositions[i].x_advance;
		y -= hbpositions[i].y_advance;
	}

	glyph->glyphs = cglyphs;
	glyph->nglyphs = nglyphs;
	glyph->width = width <= 0.0f ? glyph->fontsize : width;
	glyph->height = height <= 0.0f ? glyph->fontsize : height;

	return 0;
}

void nemotext_glyph_render(cairo_t *cr, struct nemoglyph *glyph)
{
	struct nemofont *font = glyph->font;
	cairo_font_extents_t extents;

	cairo_set_scaled_font(cr, font->cfont);
	cairo_set_font_size(cr, glyph->fontsize * font->fontscale);

	cairo_font_extents(cr, &extents);
	if (HB_DIRECTION_IS_VERTICAL(glyph->hbdirection)) {
		cairo_translate(cr, extents.height / 2.0f, -extents.descent);
	} else {
		cairo_translate(cr, 0.0f, extents.height - extents.descent);
	}

	cairo_glyph_path(cr, glyph->glyphs, glyph->nglyphs);
}
