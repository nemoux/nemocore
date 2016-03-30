#ifndef	__GL_HELPER_H__
#define	__GL_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

static const char GLHELPER_VERTEX_SHADER[] =
"uniform mat4 proj;\n"
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 vtexcoord;\n"
"void main()\n"
"{\n"
"   gl_Position = proj * vec4(position, 0.0, 1.0);\n"
"   vtexcoord = texcoord;\n"
"}\n";

#define FRAGMENT_CONVERT_YUV						\
	"  y *= alpha;\n"						\
"  u *= alpha;\n"						\
"  v *= alpha;\n"						\
"  gl_FragColor.r = y + 1.59602678 * v;\n"			\
"  gl_FragColor.g = y - 0.39176229 * u - 0.81296764 * v;\n"	\
"  gl_FragColor.b = y + 2.01723214 * u;\n"			\
"  gl_FragColor.a = alpha;\n"

static const char GLHELPER_FRAGMENT_DEBUG[] =
"  gl_FragColor = vec4(0.0, 0.3, 0.0, 0.2) + gl_FragColor * 0.8;\n";

static const char GLHELPER_FRAGMENT_BRACE[] =
"}\n";

static const char GLHELPER_TEXTURE_FRAGMENT_SHADER_RGBA[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor = alpha * texture2D(tex, vtexcoord);\n";

static const char GLHELPER_TEXTURE_FRAGMENT_SHADER_RGBX[] =
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor.rgb = alpha * texture2D(tex, vtexcoord).rgb;\n"
"   gl_FragColor.a = alpha;\n";

static const char GLHELPER_TEXTURE_FRAGMENT_SHADER_EGL_EXTERNAL[] =
"#extension GL_OES_EGL_image_external : require\n"
"precision mediump float;\n"
"varying vec2 vtexcoord;\n"
"uniform samplerExternalOES tex;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor = alpha * texture2D(tex, vtexcoord)\n;";

static const char GLHELPER_TEXTURE_FRAGMENT_SHADER_Y_UV[] =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D tex1;\n"
"varying vec2 vtexcoord;\n"
"uniform float alpha;\n"
"void main() {\n"
"  float y = 1.16438356 * (texture2D(tex, vtexcoord).x - 0.0625);\n"
"  float u = texture2D(tex1, vtexcoord).r - 0.5;\n"
"  float v = texture2D(tex1, vtexcoord).g - 0.5;\n"
FRAGMENT_CONVERT_YUV;

static const char GLHELPER_TEXTURE_FRAGMENT_SHADER_Y_U_V[] =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D tex1;\n"
"uniform sampler2D tex2;\n"
"varying vec2 vtexcoord;\n"
"uniform float alpha;\n"
"void main() {\n"
"  float y = 1.16438356 * (texture2D(tex, vtexcoord).x - 0.0625);\n"
"  float u = texture2D(tex1, vtexcoord).x - 0.5;\n"
"  float v = texture2D(tex2, vtexcoord).x - 0.5;\n"
FRAGMENT_CONVERT_YUV;

static const char GLHELPER_TEXTURE_FRAGMENT_SHADER_Y_XUXV[] =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D tex1;\n"
"varying vec2 vtexcoord;\n"
"uniform float alpha;\n"
"void main() {\n"
"  float y = 1.16438356 * (texture2D(tex, vtexcoord).x - 0.0625);\n"
"  float u = texture2D(tex1, vtexcoord).g - 0.5;\n"
"  float v = texture2D(tex1, vtexcoord).a - 0.5;\n"
FRAGMENT_CONVERT_YUV;

static const char GLHELPER_SOLID_FRAGMENT_SHADER[] =
"precision mediump float;\n"
"uniform vec4 color;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor = alpha * color;\n";

struct glshader {
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	GLint uprojection;
	GLint utextures[3];
	GLint ualpha;
	GLint ucolor;
	const char *vertex_source, *fragment_source;
};

extern GLuint glshader_compile(GLenum type, int count, const char **sources);

extern int glshader_prepare(struct glshader *shader, const char *vertex_source, const char *fragment_source, int debug);
extern void glshader_finish(struct glshader *shader);

extern GLuint glshader_create_program(const char *fshader, const char *vshader);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
