#ifndef __FONT_HELPER_H__
#define __FONT_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <hb-ft.h>
#include <hb-ot.h>
#include <fontconfig/fontconfig.h>

extern const char *fontconfig_get_path(const char *fontfamily, const char *fontstyle, int fontslant, int fontweight, int fontwidth, int fontspacing);

extern int fontconfig_get_max_advance_height(const char *fontpath, unsigned int fontindex);

extern hb_font_t *fontconfig_create_hb_font(const char *fontpath, unsigned int fontindex);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
