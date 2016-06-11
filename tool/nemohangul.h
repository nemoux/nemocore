#ifndef __NEMOTOOL_HANGUL_H__
#define __NEMOTOOL_HANGUL_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemohangul;

extern struct nemohangul *nemohangul_create(void);
extern void nemohangul_destroy(struct nemohangul *hangul);

extern void nemohangul_process(struct nemohangul *hangul, int code);
extern void nemohangul_backspace(struct nemohangul *hangul);
extern void nemohangul_reset(struct nemohangul *hangul);
extern void nemohangul_delete(struct nemohangul *hangul);

extern const uint32_t *nemohangul_get_preedit_string(struct nemohangul *hangul);
extern const uint32_t *nemohangul_get_commit_string(struct nemohangul *hangul);
extern const uint32_t *nemohangul_flush(struct nemohangul *hangul);

extern int nemohangul_is_empty(struct nemohangul *hangul);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
