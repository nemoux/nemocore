#ifndef	__GL_FILTER_H__
#define	__GL_FILTER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

static const char GLFILTER_SIMPLE_VERTEX_SHADER[] =
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"  gl_Position = vec4(position, 0.0, 1.0);\n"
"  vtexcoord = texcoord;\n"
"}\n";

static const char GLFILTER_SIMPLE_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  gl_FragColor = texture2D(tex, vtexcoord);\n"
"}\n";

static const char GLFILTER_GAUSSIAN_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * 4.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * 1.0;\n"
"  gl_FragColor = clamp(s / 16.0 + 0.0, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_LAPLACIAN_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * -4.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * 1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * 0.0;\n"
"  gl_FragColor = clamp(s / 0.1 + 0.0, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_EMBOSS_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * 2.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * 0.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * -1.0;\n"
"  gl_FragColor = clamp(s / 1.0 + 0.5, 0.0, 1.0);\n"
"}\n";

static const char GLFILTER_SHARPNESS_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float width;\n"
"uniform float height;\n"
"uniform float time;\n"
"void main()\n"
"{\n"
"  float sw = 1.0 / width;\n"
"  float sh = 1.0 / height;\n"
"  vec4 s = vec4(0.0);\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, -sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, -sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, -sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, 0.0)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, 0.0)) * 9.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, 0.0)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(-sw, sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(0.0, sh)) * -1.0;\n"
"  s += texture2D(tex, vtexcoord + vec2(sw, sh)) * -1.0;\n"
"  gl_FragColor = clamp(s / 1.0 + 0.0, 0.0, 1.0);\n"
"}\n";

extern GLuint glfilter_create_program(const char *shader);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
