#ifndef	__MINISHELL_MOTE_H__
#define	__MINISHELL_MOTE_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <showhelper.h>
#include <talehelper.h>

#include <nemomatrix.h>

struct minimote {
	struct talenode *node;

	GLuint tex;
	int32_t width, height;

	GLuint fbo, dbo;

	GLuint program;
	GLuint uprojection;
	GLuint ucolor;

	struct nemomatrix matrix;
	
	GLuint vertex_array;
	GLuint vertex_buffer;
};

extern struct minimote *minishell_mote_create(struct showone *one);
extern void minishell_mote_destroy(struct minimote *mote);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
