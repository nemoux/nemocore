#ifndef __FONT_HELPER_H__
#define __FONT_HELPER_H__

#include <fontconfig/fontconfig.h>

extern const char *fontconfig_get_path(const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing);

extern int fontconfig_get_max_advance_height(const char *fontpath, unsigned int fontindex);

extern hb_font_t *fontconfig_create_hb_font(const char *fontpath, unsigned int fontindex);

#endif
