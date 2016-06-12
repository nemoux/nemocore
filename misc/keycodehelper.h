#ifndef __KEYCODE_HELPER_H__
#define __KEYCODE_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

extern int keycode_is_backspace(uint32_t code);
extern int keycode_is_space(uint32_t code);
extern int keycode_is_enter(uint32_t code);
extern int keycode_is_tab(uint32_t code);
extern int keycode_is_delete(uint32_t code);
extern int keycode_is_insert(uint32_t code);
extern int keycode_is_shift(uint32_t code);
extern int keycode_is_ctrl(uint32_t code);
extern int keycode_is_alt(uint32_t code);
extern int keycode_is_modifier(uint32_t code);
extern int keycode_is_lock(uint32_t code);
extern int keycode_is_number(uint32_t code);
extern int keycode_is_alphabet(uint32_t code);
extern int keycode_is_direction(uint32_t code);
extern int keycode_is_normal(uint32_t code);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
