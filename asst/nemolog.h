#ifndef	__NEMO_LOG_H__
#define	__NEMO_LOG_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <stdint.h>

#ifdef NEMO_LOG_ON

extern int nemolog_open_file(const char *filepath);
extern void nemolog_close_file(void);
extern void nemolog_set_file(int fd);
extern int nemolog_open_socket(const char *socketpath);

extern int nemolog_message(const char *tag, const char *fmt, ...);
extern int nemolog_warning(const char *tag, const char *fmt, ...);
extern int nemolog_error(const char *tag, const char *fmt, ...);
extern int nemolog_check(int check, const char *tag, const char *fmt, ...);
extern void nemolog_checkpoint(void);
extern int nemolog_event(const char *tag, const char *fmt, ...);

#else

static inline int nemolog_open_file(const char *filepath) { return 0; }
static inline void nemolog_close_file(void) {}
static inline void nemolog_set_file(int fd) {}
static inline int nemolog_open_socket(const char *socketpath) {}

static inline int nemolog_message(const char *tag, const char *fmt, ...) { return 0; }
static inline int nemolog_warning(const char *tag, const char *fmt, ...) { return 0; }
static inline int nemolog_error(const char *tag, const char *fmt, ...) { return 0; }
static inline int nemolog_check(int check, const char *tag, const char *fmt, ...) { return 0; }
static inline void nemolog_checkpoint(void) {}
static inline int nemolog_event(const char *tag, const char *fmt, ...) { return 0; }

#endif

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
