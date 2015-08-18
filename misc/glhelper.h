#ifndef	__GL_HELPER_H__
#define	__GL_HELPER_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

static const char vertex_shader[] =
"uniform mat4 proj;\n"
"attribute vec2 position;\n"
"attribute vec2 texcoord;\n"
"varying vec2 v_texcoord;\n"
"void main()\n"
"{\n"
"   gl_Position = proj * vec4(position, 0.0, 1.0);\n"
"   v_texcoord = texcoord;\n"
"}\n";

#define FRAGMENT_CONVERT_YUV						\
	"  y *= alpha;\n"						\
"  u *= alpha;\n"						\
"  v *= alpha;\n"						\
"  gl_FragColor.r = y + 1.59602678 * v;\n"			\
"  gl_FragColor.g = y - 0.39176229 * u - 0.81296764 * v;\n"	\
"  gl_FragColor.b = y + 2.01723214 * u;\n"			\
"  gl_FragColor.a = alpha;\n"

static const char fragment_debug[] =
"  gl_FragColor = vec4(0.0, 0.3, 0.0, 0.2) + gl_FragColor * 0.8;\n";

static const char fragment_brace[] =
"}\n";

static const char texture_fragment_shader_rgba[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor = alpha * texture2D(tex, v_texcoord);\n";

static const char texture_fragment_shader_rgbx[] =
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform sampler2D tex;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor.rgb = alpha * texture2D(tex, v_texcoord).rgb;\n"
"   gl_FragColor.a = alpha;\n";

static const char texture_fragment_shader_egl_external[] =
"#extension GL_OES_EGL_image_external : require\n"
"precision mediump float;\n"
"varying vec2 v_texcoord;\n"
"uniform samplerExternalOES tex;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor = alpha * texture2D(tex, v_texcoord)\n;";

static const char texture_fragment_shader_y_uv[] =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D tex1;\n"
"varying vec2 v_texcoord;\n"
"uniform float alpha;\n"
"void main() {\n"
"  float y = 1.16438356 * (texture2D(tex, v_texcoord).x - 0.0625);\n"
"  float u = texture2D(tex1, v_texcoord).r - 0.5;\n"
"  float v = texture2D(tex1, v_texcoord).g - 0.5;\n"
FRAGMENT_CONVERT_YUV;

static const char texture_fragment_shader_y_u_v[] =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D tex1;\n"
"uniform sampler2D tex2;\n"
"varying vec2 v_texcoord;\n"
"uniform float alpha;\n"
"void main() {\n"
"  float y = 1.16438356 * (texture2D(tex, v_texcoord).x - 0.0625);\n"
"  float u = texture2D(tex1, v_texcoord).x - 0.5;\n"
"  float v = texture2D(tex2, v_texcoord).x - 0.5;\n"
FRAGMENT_CONVERT_YUV;

static const char texture_fragment_shader_y_xuxv[] =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"uniform sampler2D tex1;\n"
"varying vec2 v_texcoord;\n"
"uniform float alpha;\n"
"void main() {\n"
"  float y = 1.16438356 * (texture2D(tex, v_texcoord).x - 0.0625);\n"
"  float u = texture2D(tex1, v_texcoord).g - 0.5;\n"
"  float v = texture2D(tex1, v_texcoord).a - 0.5;\n"
FRAGMENT_CONVERT_YUV;

static const char solid_fragment_shader[] =
"precision mediump float;\n"
"uniform vec4 color;\n"
"uniform float alpha;\n"
"void main()\n"
"{\n"
"   gl_FragColor = alpha * color;\n";

struct glshader {
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	GLint proj_uniform;
	GLint tex_uniforms[3];
	GLint alpha_uniform;
	GLint color_uniform;
	const char *vertex_source, *fragment_source;
};

extern GLuint glshader_compile(GLenum type, int count, const char **sources);

extern int glshader_prepare(struct glshader *shader, const char *vertex_source, const char *fragment_source, int debug);
extern void glshader_finish(struct glshader *shader);

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif
