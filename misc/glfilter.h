#ifndef	__GL_FILTER_H__
#define	__GL_FILTER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

static const char simple_filter_vertex_shader[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char simple_filter_fragment_shader[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord);\n"
"}\n";

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
