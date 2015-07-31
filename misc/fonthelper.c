#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <fontconfig/fontconfig.h>

#include <fonthelper.h>

#define	FONTCONFIG_INFO_MAX			(64)
#define	FONTCONFIG_PATH_MAX			(128)
#define	FONTCONFIG_FAMILY_MAX		(32)
#define	FONTCONFIG_STYLE_MAX		(64)

struct fontinfo {
	char fontpath[FONTCONFIG_PATH_MAX];
	char fontfamily[FONTCONFIG_FAMILY_MAX];
	char fontstyle[FONTCONFIG_STYLE_MAX];
	int fontslant;
	int fontweight;
	int fontwidth;
	int fontspacing;
};

struct fontconfig {
	struct fontinfo *fonts[FONTCONFIG_INFO_MAX];
	int nfonts;

	FcConfig *fcconfig;
};

static struct fontconfig *fontconfig_create(void)
{
	struct fontconfig *config;

	config = (struct fontconfig *)malloc(sizeof(struct fontconfig));
	if (config == NULL)
		return NULL;
	memset(config, 0, sizeof(struct fontconfig));

	config->fcconfig = FcInitLoadConfigAndFonts();
	if (!FcConfigSetRescanInterval(config->fcconfig, 0))
		goto err1;

	return config;

err1:
	free(config);

	return NULL;
}

static void fontconfig_destroy(struct fontconfig *config)
{
	int i;

	FcConfigDestroy(config->fcconfig);
	FcFini();

	for (i = 0; i < config->nfonts; i++) {
		free(config->fonts[i]);
	}

	free(config);
}

static const char *fontconfig_search_path(struct fontconfig *config, const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing)
{
	FcPattern *pattern;
	FcPattern *match;
	FcFontSet *set;
	FcChar8 *filepath;
	FcResult res;
	FcBool r;
	char *path = NULL;
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

	FcConfigSubstitute(config->fcconfig, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	set = FcFontSetCreate();

	match = FcFontMatch(config->fcconfig, pattern, &res);
	if (match != NULL)
		FcFontSetAdd(set, match);

	if (match == NULL || set->nfont == 0) {
		FcFontSet *tset;

		tset = FcFontSort(config->fcconfig, pattern, FcTrue, NULL, &res);
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

	path = strdup(filepath);

out:
	FcFontSetDestroy(set);

	return path;
}

const char *fontconfig_get_path(const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing)
{
	static struct fontconfig *config = NULL;
	struct fontinfo *info;
	const char *path;
	int i;

	if (config == NULL) {
		config = fontconfig_create();
	}

	for (i = 0; i < config->nfonts; i++) {
		info = config->fonts[i];

		if (fontfamily == NULL || strcmp(info->fontfamily, fontfamily) != 0)
			continue;
		if (fontstyle == NULL || strcmp(info->fontstyle, fontstyle) != 0)
			continue;
		if (info->fontslant != fontslant)
			continue;
		if (info->fontweight != fontweight)
			continue;
		if (info->fontwidth != fontwidth)
			continue;
		if (info->fontspacing != fontspacing)
			continue;

		return info->fontpath;
	}

	path = fontconfig_search_path(config, fontfamily, fontstyle, fontslant, fontweight, fontwidth, fontspacing);
	if (path == NULL)
		return NULL;

	info = (struct fontinfo *)malloc(sizeof(struct fontinfo));
	if (info == NULL)
		return NULL;

	strcpy(info->fontpath, path);

	if (fontfamily != NULL)
		strcpy(info->fontfamily, fontfamily);
	if (fontstyle != NULL)
		strcpy(info->fontstyle, fontstyle);

	info->fontslant = fontslant;
	info->fontweight = fontweight;
	info->fontwidth = fontwidth;
	info->fontspacing = fontspacing;

	config->fonts[config->nfonts++] = info;

	return info->fontpath;
}
