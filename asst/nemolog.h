#ifndef	__NEMO_LOG_H__
#define	__NEMO_LOG_H__

#ifdef NEMO_LOG_ON

extern int nemolog_open_file(const char *filepath);
extern void nemolog_close_file(void);
extern void nemolog_set_file(FILE *file);

extern int nemolog_message(const char *tag, const char *fmt, ...);
extern int nemolog_warning(const char *tag, const char *fmt, ...);
extern int nemolog_error(const char *tag, const char *fmt, ...);

#else

static inline int nemolog_open_file(const char *filepath) { return 0; }
static inline void nemolog_close_file(void) {}
static inline void nemolog_set_file(FILE *file) {}

static inline int nemolog_message(const char *tag, const char *fmt, ...) { return 0; }
static inline int nemolog_warning(const char *tag, const char *fmt, ...) { return 0; }
static inline int nemolog_error(const char *tag, const char *fmt, ...) { return 0; }

#endif

#endif
